#include "socket.h"

void writeData(const std::string &data)
{
    if (nbiot_serial.available())
    {
        for (int i = 0; i <= data.size(); i++)
        {
            nbiot_serial.write(data[i]);
        }
        return;
    }
    throw SerialException("Can't write to serial port");
}

std::string receiveRawData()
{
    int i = 0;
    std::string out;

    while (nbiot_serial.available() && i <= MAX_MESSAGE_SIZE)
    {
        char c = nbiot_serial.read();
        out += c;
        i++;
        delay(2);
    }

    return out;
}

void connectSocket(DeviceStatus &status)
{
    std::string resp;
    switch (status.socketStatus)
    {

    case SocketStatus::SOCKET_OFF:
    {
        // Check SIM
        writeData(COMMAND_CHECK_SIM);
        resp = receiveRawData();
        ESP_LOGI("CONNECT", "Received %s", resp.c_str());

        if (!startsWith(resp, COMMAND_RESPONSE_SIM_OK))
        {
            ESP_LOGE("CONNECT", "SIM not connected");
            return;
        }
        status.socketStatus = SocketStatus::SOCKET_SIM_OK;
    }
    case SocketStatus::SOCKET_SIM_OK:
    {
        // Check signal status
        getSignalStrength(status);

        if (status.signal > MAX_SIGNAL_VALUE)
        {
            ESP_LOGE("CONNECT", "Signal is not strong enough");
            return;
        }
        status.socketStatus = SocketStatus::SOCKET_SIGNAL_OK;
    }

    case SocketStatus::SOCKET_SIGNAL_OK:
    {

        // Check service status
        writeData(COMMAND_CHECK_SERVICE);
        resp = receiveRawData();
        if (!startsWith(resp, COMMAND_RESPONSE_SIM_OK))
        {
            ESP_LOGE("CONNECT", "Invalid response for service command");
            return;
        }

        std::pair value = getValuesFromAt(resp);

        if (value.second != 1 || value.second != 5)
        {
            ESP_LOGE("CONNECT", "Failed to register to service, code %d", value.second);
            return;
        }
        status.socketStatus = SocketStatus::SOCKET_SERVICE_REGISTERED;
    }
    case SocketStatus::SOCKET_SERVICE_REGISTERED:
    {
        // Create socket
        writeData(COMMAND_CREATE_SOCKET);
        resp = receiveRawData();

        if (!startsWith(resp, COMMAND_RESPONSE_CREATED))
        {
            ESP_LOGE("CONNECT", "Failed to create socket");
            return;
        }

        // TODO: Extract the socket ID - verify
        int socketId = std::stoi(getSuffix(resp, ':'));

        std::string connect = COMMAND_CONNECT;
        connect += socketId;

        writeData(connect);
        resp = receiveRawData();

        if (resp == COMMAND_RESPONSE_OK)
        {
            ESP_LOGI("CONNECT", "Sucessfully connected to socket");
            status.socketStatus = SocketStatus::SOCKET_CONNECTED;
            status.socketId = socketId;
            return;
        }

        ESP_LOGE("CONNECT", "Failed to connected to a socket");
    }
    default:
        break;
    }
}

void sendData(const byte *data, int dataLen, int socketId)
{
    std::string buffer;
    std::string hexData = dataToHex(data, dataLen);
    buffer += COMMAND_SEND;
    buffer += socketId + "," + hexData.size();
    buffer += "," + hexData;

    writeData(buffer);

    buffer = "";
    buffer = receiveRawData();

    // Wait for positive reply
    if (buffer == COMMAND_RESPONSE_OK)
    {
        return;
    }

    // TODO: detailed error handling
    else if (startsWith(buffer, COMMAND_RESPONSE_ERROR))
    {
        throw SocketException("Error when sending data");
    }
    throw SocketException("Error when sending data - other");
}

void sendMessage(const ProtocolMessage &protocolMessage, DeviceStatus &status)
{
    std::string data = messageToString(protocolMessage);

    byte buf[MAX_MESSAGE_SIZE];
    word32 encSize;

    encryptData(data, status.key, buf, encSize);

    // Write to serial
    sendData(buf, encSize, status.socketId);
    ESP_LOGI("SENDMSG:", "Sucessfully sent data");
}

std::string getData(DeviceStatus &status)
{
    std::string received = receiveRawData();

    // Check if data doesn't exceed max message size
    if (startsWith(received, COMMAND_RESPONSE_INCOMMING_DATA) && received.size() <= (MAX_MESSAGE_SIZE + COMMAND_RESPONSE_INCOMMING_DATA.size()))
    {
        std::string trimmed = getSuffix(received, ':'); // Trim the message indicator
        byte rawData[MAX_MESSAGE_SIZE];
        hexToData(trimmed, rawData);
        std::string out;

        decryptData(rawData, (trimmed.size() / 2), status.key, out);
        ESP_LOGI("GETDATA:", "Sucessfully received data");

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
            if (validateSignature(sigBytes, sigLength, "0", status.serverKey))
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
    std::string out = COMMAND_CLOSE + std::to_string(status.socketId);
    writeData(out);
}

void getSignalStrength(DeviceStatus &status)
{
    writeData(COMMAND_CHECK_SIGNAL);
    std::string response = receiveRawData(); // Format +CSQ: <rssi>,<ber>

    if (startsWith(response, COMMAND_RESPONSE_SIGNAL))
    {
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
            return;
        }
    }
    throw SocketException("Invalid signal strength response");
}