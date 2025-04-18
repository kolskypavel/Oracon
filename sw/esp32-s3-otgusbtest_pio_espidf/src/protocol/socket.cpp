#include "socket.h"

void writeData(const std::string &data)
{
    nbiot_serial.println(data.c_str());
    ESP_LOGI("SOCKET", "Wrote data: %s", data.c_str());
    delay(100);
}

std::string receiveRawData()
{
    int i, timeout = 0;
    std::string out;

    while (nbiot_serial.available() == 0)
    {
        if (timeout >= SOCKET_READ_TIMEOUT)
        {
            throw SocketException("Socket timeout expired");
        }
        ESP_LOGI("SOCKET", "TIMEOUT %d", timeout);
        timeout++;
        delay(1000);
    }

    while (nbiot_serial.available())
    {
        // if (i >= MAX_MESSAGE_SIZE)
        // {
        //     // Clear incoming buffer
        //     while (Serial.available())
        //     {
        //         Serial.read();
        //     }

        //     throw std::invalid_argument("Message length exceeded maximal size");
        // }

        char c = nbiot_serial.read();
        out += c;
        i++;
    }

    trimmString(out); // Trim leading/trailing whitespaces
    ESP_LOGI("DATA", "Received: %s", out.c_str());
    return out;
}

void initSocket(DeviceStatus &status)
{
    std::string resp;

    // Check SIM
    writeData(COMMAND_CHECK_SIM);
    resp = receiveRawData();

    if (!startsWith(resp, COMMAND_RESPONSE_SIM_OK))
    {
        throw std::runtime_error("SIM not connected");
    }

    // Disable ip output when receiving data
    writeData(COMMAND_DISABLE_IP_OUTPUT);
    resp = receiveRawData();

    if (!startsWith(resp, COMMAND_RESPONSE_OK))
    {
        throw std::runtime_error("Failed to disable IP output");
    }
    ESP_LOGI("CONNECT", "Socket init successful");
}

void connectSocket(DeviceStatus &status)
{
    std::string resp;

    // Create socket
    writeData(COMMAND_CREATE_SOCKET);
    resp = receiveRawData();

    if (!startsWith(resp, COMMAND_RESPONSE_OK) && !startsWith(resp, COMMAND_RESPONSE_SOCKET_EXISTING))
    {
        ESP_LOGE("CONNECT", "Failed to create socket");
        return;
    }

    std::string connect = COMMAND_CONNECT;
    connect += "0,\"TCP\",";
    connect += SERVER_IP;
    connect += ",";
    connect += SERVER_PORT;

    writeData(connect);
    delay(1000);
    delay(CONNECT_TIMEOUT * 1000);
    resp = receiveRawData();

    std::pair<int, int> values = getValuesFromAt(resp);

    if (values.second == 0)
    {
        ESP_LOGI("CONNECT", "Sucessfully connected to socket");
        status.socketStatus = SocketStatus::SOCKET_CONNECTED;
        return;
    }
    else
    {
        ESP_LOGE("CONNECT", "Failed to connect, cause %s", getCause(values.second));
    }
}

void sendData(const byte *data, int dataLen, int socketId)
{
    std::string buffer;
    std::string hexData = dataToHex(data, dataLen);
    hexData += "\n";

    buffer += COMMAND_SEND;
    buffer += std::to_string(socketId);
    buffer += ",";
    buffer += std::to_string(hexData.size());

    // Send sending command
    writeData(buffer);
    buffer = receiveRawData();

    if (buffer != COMMAND_RESPONSE_SEND)
    {
        throw SocketException("Invalid response to send command:" + buffer);
    }

    // Send actual data
    writeData(hexData);

    buffer = "";
    buffer = receiveRawData();

    // Wait for positive reply
    if (startsWith(buffer, COMMAND_RESPONSE_OK))
    {
        return;
    }

    // TODO: detailed error handling
    else if (startsWith(buffer, COMMAND_RESPONSE_ERROR))
    {
        throw SocketException("Error when sending data");
    }
}

void sendMessage(const ProtocolMessage &protocolMessage, DeviceStatus &status)
{
    std::string data = messageToString(protocolMessage);

    // byte buf[MAX_MESSAGE_SIZE];
    // word32 encSize;

    // encryptData(data, status.key, buf, encSize);

    // Write to serial
    // sendData(buf, encSize, status.socketId);
    sendData(reinterpret_cast<const byte *>(data.data()), data.size(), status.socketId);
    ESP_LOGI("SENDMSG", "Sucessfully sent data");
}

std::string getData(DeviceStatus &status)
{
    std::string received = receiveRawData();

    // Check if data doesn't exceed max message size
    if (startsWith(received, COMMAND_RESPONSE_INCOMMING_DATA) && received.size() <= (MAX_MESSAGE_SIZE + COMMAND_RESPONSE_INCOMMING_DATA.size()))
    {
        std::string trimmed = getSuffix(received, "\r\n"); // Trim the message indicator
        // uint16_t dataSize = 0;
        // std::string sizeString = getPrefix(trimmed, "\r");
        // trimmed = getSuffix(trimmed, "\r\n");

        byte rawData[MAX_MESSAGE_SIZE];
        hexToData(trimmed, rawData);
        std::string out;

        decryptData(rawData, (trimmed.size() / 2), status.key, out);
        ESP_LOGI("GETDATA:", "Sucessfully received data");
        Serial.print(out.c_str());

        return out;
    }

    throw std::invalid_argument("Invalid format when receiving data");
}

bool validateMessage(const ProtocolMessage &msg, DeviceStatus &status)
{
    if (msg.token == status.token)
    {
        return true;
    }
    return false;
}

ProtocolMessage getNewMessage(DeviceStatus &status)
{
    std::string received = getData(status);
    ProtocolMessage msg = parseMessage(received);

    if (validateMessage(msg, status))
    {
        return msg;
    }
    throw std::invalid_argument("Received message is invalid");
}

void initMessage(ProtocolMessage &msg, DeviceStatus &status)
{
    msg.deviceId = status.deviceId;
    msg.token = status.token;
}

void sendAck(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_ACK;
    return sendMessage(msg, status);
}

void sendNack(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_NACK;

    sendMessage(msg, status);
}

void authenticateDevice(DeviceStatus &status)
{
    // Send connect message
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_CONNECT;
    msg.data = generateSignatureData(status); // Add signature

    sendMessage(msg, status);
    ESP_LOGI("AUTH:", "Connect sent");

    try
    {
        // Wait for connect response
        msg = getNewMessage(status);

        if (msg.type == ProtocolMessageType::TYPE_CONNECT)
        {
            ESP_LOGI("AUTH:", "Connect message received");

            std::string signature = dataToSignature(msg.data);
            byte sigBytes[MAX_SIGNATURE_SIZE];

            word32 sigLength = signature.size() / 2; // Hex encoded string - actual size is half
            hexToData(signature, sigBytes);

            // Server ID should be always 0
            if (verifySignature(sigBytes, sigLength, "0", status.serverKey))
            {
                ESP_LOGI("AUTH:", "Sucessfully authenticated device");

                sendAck(status);
                status.socketStatus = SocketStatus::SOCKET_AUTHENTICATED;
                return;
            }
            ESP_LOGE("AUTH:", "Failed to verify server signature");
        }
        ESP_LOGE("AUTH:", "Connect message not received");
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("AUTH:", "Error: %s", ex.what());
    }

    // Throw exception to terminate socket connection
    throw SocketException("Failed to authenticate device");
}

void sendStatus(DeviceStatus &status, Preferences prefs)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_STATUS;
    msg.data = statusToString(status);

    sendMessage(msg, status);

    msg = getNewMessage(status);

    if (msg.type == ProtocolMessageType::TYPE_CONF)
    {
        ESP_LOGI("STATUS:", "Received config message");
        try
        {
            DeviceConfig config = stringToConfig(msg.data);
            status.config = config;

            // Save to device flash memory -> persistent after reboot
            prefs.putUChar("statusDelay", config.statusDelay);
            sendAck(status);
        }
        // Error when receiving configuration
        catch (const std::invalid_argument &exception)
        {
            ESP_LOGE("STATUS:", "Failed to parse config message");
            sendNack(status);
        }
    }
    else if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        // Everything OK
        ESP_LOGI("STATUS:", "Status received by server");
        return;
    }
    else
    {
        // Undefined behavior
        throw SocketException("Undefined behavior when receiving status");
    }
}

bool sendPunches(DeviceStatus &status, SIRecord punches[], int punchCount)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_PUNCH;
    msg.data = punchesToString(punches, punchCount);

    ESP_LOGI("PUNCH:", "Sending %d punches", punchCount);
    sendMessage(msg, status);

    msg = getNewMessage(status);

    // Get confirmation
    if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        ESP_LOGI("PUNCH:", "Punches successfully received by server");
        return true;
    }

    ESP_LOGE("PUNCH:", "Punches not received by server");
    return false;
}

void closeSocket(DeviceStatus &status)
{
    writeData(COMMAND_CLOSE);
    std::string resp = receiveRawData();
    if (startsWith(resp, COMMAND_RESPONSE_OK))
    {
        ESP_LOGI("SOCKET:", "Socket closed successfuly");
        return;
    }
    ESP_LOGE("SOCKET:", "Failed to close socket");
}

void getSignalStrength(DeviceStatus &status)
{
    writeData(COMMAND_CHECK_SIGNAL);
    std::string response = receiveRawData(); // Format +CSQ: <rssi>,<ber>

    if (!startsWith(response, COMMAND_RESPONSE_SIGNAL))
    {
        throw SocketException("Invalid signal strength response");
    }

    std::pair values = getValuesFromAt(response);
    int rssi = values.first;

    // NB-Iot signal not detectable
    if (rssi == 99)
    {
        status.signal = 0;
    }
    // Convert RSSI to dBm using TS 27.007 Section 8.5
    else if (rssi >= 0 && rssi <= 31)
    {
        status.signal = 113 - (rssi * 2); // dBm calculation
    }

    // Check service status
    writeData(COMMAND_CHECK_SERVICE);
    response = receiveRawData();
    if (!startsWith(response, COMMAND_RESPONSE_SERVICE))
    {
        throw SocketException("Invalid response for service command");
    }

    std::pair value = getValuesFromAt(response);
    // ESP_LOGI("VALUE", "FIRST %d", value.first);
    // ESP_LOGI("VALUE", "SECOND %d", value.second);

    if (value.second != 1 && value.second != 5)
    {
        throw SocketException("Failed to register to service, code" + value.second);
    }
}