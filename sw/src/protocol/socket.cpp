#include "socket.h"

void writeData(const std::string &data)
{
    nbiot_serial.println(data.c_str());

#ifdef TEST_ORACON_SERIAL_VERBOSE
    ESP_LOGI("SOCKET", "Wrote data: %s", data.c_str());
#endif

    delay(100);
}

void clearInBuffer()
{
    while (nbiot_serial.available())
    {
        nbiot_serial.read();
    }
}

std::string receiveRawData()
{
    long i = 0;
    int timeout = 0;
    std::string out = "";

    while (nbiot_serial.available() == 0)
    {
        if (timeout >= SOCKET_READ_TIMEOUT)
        {
            throw SocketException("Socket read timeout expired");
        }

#ifdef TEST_ORACON_SERIAL_VERBOSE
        ESP_LOGI("SOCKET", "TIMEOUT %d", timeout);
#endif

        timeout++;
        delay(1000);
    }

    while (nbiot_serial.available())
    {
#ifdef LIMIT_NB_IOT_SERIAL
        if (i >= MAX_MESSAGE_SIZE)
        {
            // Clear incoming buffer
            while (Serial.available())
            {
                Serial.read();
            }

            throw std::invalid_argument("Message length exceeded maximal size");
        }
#endif

        char c = nbiot_serial.read();
        out += c;
        i++;
    }

#ifdef TEST_ORACON_SERIAL_VERBOSE
    ESP_LOGI("DATA", "Received %ld B: %s", i, out.c_str());
#endif

    trimmString(out); // Trim leading/trailing whitespaces
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

    // Set socket to buffer receiving data
    writeData(COMMAND_RECEIVE_DATA + "1");
    resp = receiveRawData();

    if (!startsWith(resp, COMMAND_RESPONSE_OK))
    {
        throw std::runtime_error("Failed to set buffered output");
    }

#ifndef TEST_ORACON_NO_TIMEOUT
    std::string data = COMMAND_SET_TIMEOUT;
    data += std::to_string(SOCKET_OPEN_TIMEOUT * 1000) +
            "," + std::to_string(SOCKET_CONNECT_TIMEOUT * 1000) +
            "," + std::to_string(SOCKET_READ_TIMEOUT * 1000);

    writeData(data);
    resp = receiveRawData();
    if (!startsWith(resp, COMMAND_RESPONSE_OK))
    {
        throw std::runtime_error("Failed to set timeouts");
    }
#endif

    ESP_LOGI("CONNECT", "Socket init successful");
}

void connectSocket(DeviceStatus &status)
{
    std::string resp;

    // Check netopen status
    writeData(COMMAND_CREATE_SOCKET + "?");
    resp = receiveRawData();

    if (!startsWith(resp, COMMAND_RESPONSE_SOCKET_EXISTING))
    {
        writeData(COMMAND_CREATE_SOCKET);
        resp = receiveRawData();

        if (!startsWith(resp, COMMAND_RESPONSE_OK))
        {
            ESP_LOGE("CONNECT", "Failed to create socket");
            return;
        }
    }

    // Check socket connection status
    writeData(COMMAND_CONNECT + "?");
    resp = receiveRawData();

    if (startsWith(resp, COMMAND_RESPONSE_CONNECTED))
    {
        ESP_LOGI("CONNECT", "Device already connected to socket");
        status.socketStatus = SocketStatus::SOCKET_CONNECTED;
        return;
    }

    std::string connect = COMMAND_CONNECT;
    connect += "=0,\"TCP\",";
    connect += SERVER_IP;
    connect += ",";
    connect += SERVER_PORT;

    writeData(connect);
    delay(SOCKET_CONNECT_READ_TIMEOUT * 1000);
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

    if (startsWith(buffer, COMMAND_RESPONSE_SEND_ERROR))
    {
        throw SocketException("Socket not open / closed by server");
    }
    else if (buffer != COMMAND_RESPONSE_SEND)
    {
        throw std::invalid_argument("Invalid response to send command:" + buffer);
    }

    // Send actual data
    writeData(hexData);

    buffer = receiveRawData();

    // Wait for positive reply
    if (startsWith(buffer, COMMAND_RESPONSE_OK))
    {
        return;
    }
    else if (startsWith(buffer, COMMAND_RESPONSE_CLOSE_SOCKET))
    {
        throw SocketException("Socket closed by server");
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

#ifdef TEST_ORACON_NO_ENCRYPTION
    sendData(reinterpret_cast<const byte *>(data.data()), data.size(), status.socketId);
#else
    byte buf[MAX_MESSAGE_SIZE];
    word32 encSize;

    encryptDataAes(data, status.aesKey, buf, encSize);
    sendData(buf, encSize, status.socketId);
#endif

    ESP_LOGI("SENDMSG", "Sucessfully sent data");
}

std::string getData(DeviceStatus &status)
{
    std::string received = receiveRawData();

    if (startsWith(received, COMMAND_RESPONSE_INCOMMING_DATA))
    {
        std::string buffer, trimmed = "";
        int remaining = 0;

        do
        {
            writeData(COMMAND_RECEIVE_DATA + SOCKET_READ_MODE + ",0," + std::to_string(SOCKET_READ_SIZE));
            received = receiveRawData();
            std::string header = getSubstr(received, COMMAND_RESPONSE_INCOMMING_DATA, "\r\n");
            remaining = std::stoi(getSuffix(header, ","));

            trimmed = getSubstr(received, "\r\n", "\r\nOK"); // Trim the message indicator
            trimmString(trimmed);
            buffer += trimmed;

        } while (remaining > 0);

        std::string out;
        byte rawData[MAX_MESSAGE_SIZE];
        hexToData(buffer, rawData);

#ifdef TEST_ORACON_NO_ENCRYPTION
        out = std::string(reinterpret_cast<const char *>(rawData), buffer.size() / 2);
#else
        decryptDataAes(rawData, (buffer.size() / 2), status.aesKey, out);
#endif
        ESP_LOGI("GETDATA", "Sucessfully received data %s", out.c_str());
        return out;
    }
    else if (startsWith(received, COMMAND_RESPONSE_CLOSE_SOCKET))
    {
        throw SocketException("Socket closed by server");
    }

    throw std::invalid_argument("Invalid format when receiving data");
}

ProtocolMessage getNewMessage(DeviceStatus &status)
{
    std::string received = getData(status);
    ProtocolMessage msg = parseMessage(received);
    return msg;
}

void initMessage(ProtocolMessage &msg, DeviceStatus &status)
{
    msg.deviceId = status.deviceId;
}

void sendAck(DeviceStatus &status)
{
    ESP_LOGI("ACK", "Sending ACK");
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_ACK;
    return sendMessage(msg, status);
}

void sendNack(DeviceStatus &status)
{
    ESP_LOGI("NACK", "Sending NACK");
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_NACK;

    sendMessage(msg, status);
}

void verifyServer(const std::string &data, DeviceStatus &status)
{
    std::string signature = dataToSignature(data);
    byte buff[MAX_SIGNATURE_SIZE];

    hexToData(signature, buff);
    word32 buffLength = signature.size() / 2; // Hex encoded string - actual size is half

    // Server ID should be always 0
    if (!verifySignature("0", *status.serverKey, buff, buffLength))
    {
        throw std::invalid_argument("Failed to verify server signature");
    }
}

void authenticateDevice(DeviceStatus &status)
{
#ifndef TEST_ORACON_NO_ENCRYPTION
    // Generate and send AES key
    generateAesKey(status.aesKey);
    ESP_LOGI("AES KEY", "%s", dataToHex(status.aesKey, AES_KEY_SIZE).c_str());
    byte encrypted[MAX_MESSAGE_SIZE];
    int encSize = MAX_MESSAGE_SIZE;
    encryptDataRsa(status.aesKey, AES_KEY_SIZE, *status.serverKey, encrypted, encSize);
    sendData(encrypted, encSize, status.socketId);
#endif

    // Send connect message
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_CONNECT;
    msg.data = generateSignatureData(status);

    sendMessage(msg, status);
    ESP_LOGI("AUTH", "Connect sent");

    try
    {
        // Wait for connect response
        msg = getNewMessage(status);

        if (msg.type == ProtocolMessageType::TYPE_CONNECT)
        {
            ESP_LOGI("AUTH", "Connect message received");

#ifdef TEST_ORACON_NO_SIGNATURE_VERIFICATION
            sendAck(status);
            status.socketStatus = SocketStatus::SOCKET_AUTHENTICATED;
            ESP_LOGI("AUTH", "Sucessfully authenticated device");
            return;
#else
            verifyServer(msg.data, status);
            sendAck(status);
            status.socketStatus = SocketStatus::SOCKET_AUTHENTICATED;
            ESP_LOGI("AUTH", "Sucessfully authenticated device");
            return;
#endif
        }
        ESP_LOGE("AUTH", "Connect message not received");
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("AUTH", "Error: %s", ex.what());
    }

    // Throw exception to terminate socket connection
    throw SocketException("Failed to authenticate device");
}

void sendStatus(DeviceStatus &status, Preferences &prefs)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_STATUS;
    msg.data = statusToString(status);

    sendMessage(msg, status);

    msg = getNewMessage(status);

    if (msg.type == ProtocolMessageType::TYPE_CONF)
    {
        ESP_LOGI("STATUS", "Received config message");
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
            ESP_LOGE("STATUS", "Failed to parse config message");
            sendNack(status);
        }
    }
    else if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        // Everything OK
        ESP_LOGI("STATUS", "Status received by server");
        return;
    }
    // Status not accepted by server
    else
    {
        throw SocketException("Status not received by server - NACK");
    }
}

bool sendPunches(DeviceStatus &status, SIRecord punches[], int punchCount)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_PUNCH;
    msg.data = punchesToString(punches, punchCount);

    ESP_LOGI("PUNCH", "Sending %d punches", punchCount);
    sendMessage(msg, status);

    msg = getNewMessage(status);

    // Get confirmation
    if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        ESP_LOGI("PUNCH", "Punches successfully received by server");
        return true;
    }

    ESP_LOGE("PUNCH", "Punches not received by server");
    return false;
}

void closeSocket(DeviceStatus &status)
{
    writeData(COMMAND_CLOSE);
    clearInBuffer();
}

void getSignalStrength(DeviceStatus &status)
{
    writeData(COMMAND_CHECK_SIGNAL);
    std::string response = receiveRawData(); // Format +CSQ: <rssi>,<ber>

    if (!startsWith(response, COMMAND_RESPONSE_SIGNAL))
    {
        throw std::invalid_argument("Invalid signal strength response");
    }

    std::pair values = getValuesFromAt(response);
    int rssi = values.first;

    // NB-Iot signal not detectable
    if (rssi == 99)
    {
        status.signal = 113;
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
        throw std::invalid_argument("Invalid response for service command");
    }

    std::pair value = getValuesFromAt(response);

    if (value.second != 1 && value.second != 5)
    {
        throw std::invalid_argument("Failed to register to service, code" + value.second);
    }
}