#include "socket.h"

bool sendData(const byte *data)
{
    // Write data to serial

    // Compare with commands
    //  if(response == COMMAND_OK){
    //      return true;
    //  }

    return false;
}

bool sendMessage(ProtocolMessage protocolMessage)
{
    std::string data = messageToString(protocolMessage);
    // return sendData(data.);

    // TODO: encrypt

    //Write to serial
   // return sendData(encrypted);
}

std::string receiveData()
{
}

ProtocolMessage getNewMessage()
{
    std::string received = receiveData();
    return parseMessage(received);
}

bool sendAck()
{
    ProtocolMessage msg;
    msg.type = ProtocolMessageType::ACK;
    return sendData(messageToString(msg));
}

bool sendNack()
{
    ProtocolMessage msg;
    msg.type = ProtocolMessageType::NACK;
    return sendData(messageToString(msg));
}

bool sendStatus(Status & status)
{
    std::string data = statusToString(status);
    
}

bool sendPunches(SIRecord[]) {}