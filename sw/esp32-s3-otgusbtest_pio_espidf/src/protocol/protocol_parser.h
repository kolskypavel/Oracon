// Parser class for the protocol messages
#pragma once

#include "protocol_message.h"
#include "system/systemstats.h"
#include "si/si_parser.h"

// Converts given data to HEX
std::string dataToHex(const byte *data, int dataLen);

ProtocolMessage parseMessage(const std::string &message);

bool validateMessage(const ProtocolMessage &message);

std::string messageToString(const ProtocolMessage &message);

std::string messageTypeToString(ProtocolMessageType type);

ProtocolMessageType stringToMessageType(const std::string &typeString);

// Serializes a given status object to OraCon format
std::string statusToString(const DeviceStatus &status, const DeviceConfig &config);

// Serializes a given punch object to OraCon format
std::string punchToString(const SIRecord &record);

std::string punchesToString(const SIRecord punches[], int size);

DeviceConfig dataToConfig(const std::string &data);