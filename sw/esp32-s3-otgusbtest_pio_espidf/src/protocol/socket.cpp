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

bool sendData(const byte *data, int dataLen, int socketId)
{
    std::string out;
    std::string hexData = dataToHex(data, dataLen);
    out += COMMAND_SEND;
    out += socketId + "," + hexData.size();
    out += "," + hexData;

    writeData(out);

    // Wait for positive reply

    //  if(response == COMMAND_OK){
    //      return true;
    //  }
    return false;
}

bool sendMessage(const ProtocolMessage &protocolMessage)
{
    std::string data = messageToString(protocolMessage);
    // return sendData(data.);

    // TODO: encrypt

    // Write to serial
    // return sendData(encrypted);
    return false;
}

std::string receiveData()
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

ProtocolMessage getNewMessage()
{
    std::string received = receiveData();
    return parseMessage(received);
}

bool sendAck()
{
    ProtocolMessage msg;
    msg.type = ProtocolMessageType::TYPE_ACK;
    return sendMessage(msg);
}

bool sendNack()
{
    ProtocolMessage msg;
    msg.type = ProtocolMessageType::TYPE_NACK;
    return sendMessage(msg);
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