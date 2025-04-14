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
    // Create socket
    writeData(COMMAND_CREATE_SOCKET);

    std::string resp = receiveRawData();

    if (startsWith(resp, COMMAND_RESPONSE_CREATED))
    {
        // TODO: Extract the socket ID - verify
        int socketId = std::stoi(getSuffix(resp, ':'));

        std::string connect = COMMAND_CONNECT;
        connect += socketId;

        writeData(connect);
        resp = receiveRawData();

        if (resp == COMMAND_RESPONSE_OK)
        {
            status.connected = true;
            status.socketId = socketId;
        }
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
        throw SocketException("Failed to send data");
    }
}

void sendMessage(const ProtocolMessage &protocolMessage, DeviceStatus &status)
{
    std::string data = messageToString(protocolMessage);

    byte buf[MAX_MESSAGE_SIZE];
    word32 encSize;

    encryptData(data, status.key, buf, encSize);

    // Write to serial
    sendData(buf, encSize, status.socketId);
}

std::string getData(DeviceStatus &status)
{
    std::string received = receiveRawData();

    // Check if data doesn't exceed max message size
    if (startsWith(received, COMMAND_INCOMMING_DATA) && received.size() <= (MAX_MESSAGE_SIZE + COMMAND_INCOMMING_DATA.size()))
    {
        std::string trimmed = getSuffix(received, ':'); // Trim the message indicator
        byte rawData[MAX_MESSAGE_SIZE];
        hexToData(trimmed, rawData);
        std::string out;

        if (decryptData(rawData, (trimmed.size() / 2), status.key, out))
        {
            return out;
        }
        throw std::invalid_argument("Failed to decrypt data");
    }

    throw std::invalid_argument("Invalid format when receiving data");
}

bool validateMessage(const ProtocolMessage &msg, DeviceStatus &status)
{
    // TODO: fix counter
    if (msg.counter == status.counter && msg.token == status.token)
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
        status.counter++; // TODO: fix counter
        return msg;
    }
    throw std::invalid_argument("Invalid message");
}

void initMessage(ProtocolMessage &msg, DeviceStatus &status)
{
    msg.deviceId = status.deviceId;
    msg.counter = ++status.counter;
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

    try
    {
        // Wait for connect response
        msg = getNewMessage(status);

        if (msg.type == ProtocolMessageType::TYPE_CONNECT)
        {
            std::string signature = dataToSignature(msg.data);
            byte sigBytes[100]; // TODO: Init

            word32 sigLength = signature.size() / 2; // Hex encoded string - actual size is half
            hexToData(signature, sigBytes);

            // Server ID should be always 0
            if (validateSignature(sigBytes, sigLength, "0", status.serverKey))
            {
                sendAck(status);
                status.authenticated = true;
                return;
            }
        }
    }
    catch (const std::invalid_argument &a)
    {
    }

    // Throw exception to terminate socket connection
    throw SocketException("Failed to authenticate device");
}

void sendStatus(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_STATUS;
    msg.data = statusToString(status);

    sendMessage(msg, status);

    msg = getNewMessage(status);

    if (msg.type == ProtocolMessageType::TYPE_CONF)
    {
        try
        {
            DeviceConfig config = stringToConfig(msg.data);
            status.config = config;
            sendAck(status);
        }
        // Error when receiving configuration
        catch (const std::invalid_argument &exception)
        {
            sendNack(status);
        }
    }
    else if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        // Everything OK
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

    sendMessage(msg, status);

    msg = getNewMessage(status);

    // Get confirmation
    if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        return true;
    }

    return false;
}

void closeSocket(DeviceStatus &status)
{
    std::string out = COMMAND_CLOSE + std::to_string(status.socketId);
    writeData(out);
}

void getSignalStrength(DeviceStatus &status)
{
    writeData(COMMAND_SIGNAL);
    std::string response = receiveRawData(); // Format +CSQ: <rssi>,<ber>

    if (startsWith(response, COMMAND_RESPONSE_SIGNAL))
    {
        std::string trimmed = getSuffix(response, ':');
        size_t commaPos = trimmed.find(',');
        if (commaPos != std::string::npos)
        {
            std::string rssiStr = trimmed.substr(0, commaPos);
            int rssi = std::stoi(rssiStr);

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
        }
    }
    throw std::invalid_argument("Invalid signal strength response");
}