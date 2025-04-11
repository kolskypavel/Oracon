#include "socket.h"

bool startsWith(std::string text, std::string prefix)
{
    if (text.rfind(prefix, 0) == 0)
    {
        return true;
    }
    return false;
}

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
        int socketId = 0;

        std::string connect = COMMAND_CONNECT;
        connect += socketId;

        writeData(connect);
        resp = receiveRawData();

        if (resp == COMMAND_RESPONSE_OK)
        {
            status.connected = true;
            status.socketId = socketId;
            return;
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
    buffer = getData();

    // Wait for positive reply
    if (buffer == COMMAND_RESPONSE_OK)
    {
        return true;
    }

    // TODO: error handling
    return false;
}

bool sendMessage(const ProtocolMessage &protocolMessage)
{
    std::string data = messageToString(protocolMessage);

    // TODO: encrypt

    // Write to serial
    // return sendData(encrypted);
    return false;
}

ProtocolMessage getNewMessage()
{
    std::string received = getData();
    return parseMessage(received);
}

bool sendAck(DeviceStatus &status)
{
    ProtocolMessage msg;
    msg.type = ProtocolMessageType::TYPE_ACK;
    return sendMessage(msg);
}

bool sendNack(DeviceStatus &status)
{
    ProtocolMessage msg;
    msg.type = ProtocolMessageType::TYPE_NACK;
    return sendMessage(msg);
}

void authenticateDevice(DeviceStatus &status)
{
    ProtocolMessage msg;
}

bool sendStatus(DeviceStatus &status, DeviceConfig &config)
{
    std::string data = statusToString(status, config);

    return false;
}

bool sendPunches(std::vector<SIRecord> records)
{
    std::string out;
    for (SIRecord record : records)
    {
        out += punchToString(record);
    }
    ProtocolMessage msg;
    return false;
}

int getSignalStrength()
{
    return 0;
}