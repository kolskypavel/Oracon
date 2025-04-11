#include "socket.h"

bool writeData(const std::string &data)
{
    if (nbiot_serial.available())
    {
        for (int i = 0; i <= data.size(); i++)
        {
            nbiot_serial.write(data[i]);
        }
        return true;
    }
    return false;
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

bool sendData(const byte *data, int dataLen, int socketId)
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
        return true;
    }

    // TODO: error handling
    return false;
}

bool sendMessage(const ProtocolMessage &protocolMessage, DeviceStatus &status)
{
    std::string data = messageToString(protocolMessage);

    // TODO: encrypt
    byte buf[MAX_MESSAGE_SIZE];
    word32 encSize;

    encryptData(data, status.key, buf, encSize);

    // Write to serial
    return sendData(buf, encSize, status.socketId);
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
}

void initMessage(ProtocolMessage &msg, DeviceStatus &status)
{
    msg.deviceId = status.deviceId;
    msg.counter = ++status.counter;
    msg.token = status.token;
}

bool sendAck(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_ACK;
    return sendMessage(msg, status);
}

bool sendNack(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_NACK;

    return sendMessage(msg, status);
}

void authenticateDevice(DeviceStatus &status)
{
    // Send connect message
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_CONNECT;

    // Wait for connect response
    msg = getNewMessage(status);

    // Send ACK/NACK
}

bool sendStatus(DeviceStatus &status)
{
    ProtocolMessage msg;
    initMessage(msg, status);
    msg.type = ProtocolMessageType::TYPE_STATUS;
    msg.data = statusToString(status);

    return false;
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