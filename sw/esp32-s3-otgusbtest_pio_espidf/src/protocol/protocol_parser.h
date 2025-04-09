// Parser class for the protocol messages

#include "protocol_message.h"
#include "system/systemstats.h"
#include "si/si_parser.h"

struct Config
{
    int statusDelay;
};

ProtocolMessage parseMessage(const std::string & message);

bool validateMessage(const ProtocolMessage & message);

std::string messageToString(const ProtocolMessage & message);

std::string messageTypeToString(ProtocolMessageType type);

ProtocolMessageType stringToMessageType(const std::string &typeString);

// Serializes a given status object to OraCon format
std::string statusToString(const Status &status, const Config &config);

std::string punchToString(const SIRecord &record);

Config dataToConfig(const std::string & config);