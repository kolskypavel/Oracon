/*
Implementation of the ORacon protocol
*/

#include "protocol_message.h"
#include "protocol_parser.h"
#include "serial_commands.cpp"
#include "si/si_parser.h"
#include <string>

bool sendData(const byte * data);

bool sendMessage(ProtocolMessage &protocolMessage);

std::string receiveData();

ProtocolMessage getNewMessage();

bool sendAck();

bool sendNack();

bool sendStatus(Status & status);

bool sendPunches(SIRecord[]);

int getSignalStrength();

