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
        // TODO: Extract the socket ID
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

    // TODO: error handling
}

void sendMessage(const ProtocolMessage &protocolMessage, DeviceStatus &status)
{
    // TODO: add counter

    std::string data = messageToString(protocolMessage);

    // TODO: encrypt
    byte buf[MAX_MESSAGE_SIZE];
    word32 encSize;

    encryptData(data, status.key, buf, encSize);

    // Write to serial
    sendData(buf, encSize, status.socketId);
}

std::string getData()
{
    std::string received = receiveRawData();
}

bool validateMessage(const ProtocolMessage &msg, DeviceStatus &status)
{
    if (msg.counter == status.counter && msg.token == status.token)
    {
        return true;
    }
    return false;
}

ProtocolMessage getNewMessage(DeviceStatus &status)
{
    std::string received = getData();
    ProtocolMessage msg = parseMessage(received);

    // TODO: add validation
    if (validateMessage(msg, status))
    {
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
            std::string signature = stringToSignature(msg.data);
            byte sigBytes[100]; // TODO: Init

            word32 sigLength = signature.size() / 2; // Hex encoded string - actual size is half
            hexToData(signature, sigBytes);

            //Server ID should be always 0
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

bool sendStatus(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_STATUS;
    msg.data = statusToString(status);

    sendMessage(msg, status);
    msg = getNewMessage(status);

    if (msg.type == ProtocolMessageType::TYPE_ACK)
    {
        return true;
    }
    return false;
}

void processStatus(DeviceStatus &status)
{
    sendStatus(status);
    ProtocolMessage msg = getNewMessage(status);

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
    writeData("AT" + COMMAND_SIGNAL);
    std::string response = receiveRawData();

    // Parse the response
    uint8_t signal;

    status.signal = signal;
}