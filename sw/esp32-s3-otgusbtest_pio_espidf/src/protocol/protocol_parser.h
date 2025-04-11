// Parser class for the protocol messages
#pragma once

#include "protocol_message.h"
#include "system/systemstats.h"
#include "si/si_parser.h"
#include "crypto/encryptor.h"
#include <sstream>
#include <iomanip>

// Converts given data to HEX
std::string dataToHex(const byte *data, int dataLen);

// Converts given HEX to byte data
void hexToData(std::string in, byte *out);

ProtocolMessage parseMessage(const std::string &message);

bool validateMessage(const ProtocolMessage &message);

std::string messageToString(const ProtocolMessage &message);

std::string messageTypeToString(ProtocolMessageType type);

ProtocolMessageType stringToMessageType(const std::string &typeString);

// Serializes a given status object to OraCon format
std::string statusToString(const DeviceStatus &status);

// Serializes a given punch object to OraCon format
std::string punchToString(const SIRecord &record);

std::string punchesToString(const SIRecord punches[], int size);

DeviceConfig dataToConfig(const std::string &data);

// Checks if text starts with given prefix
bool startsWith(std::string text, std::string prefix);

// Gets suffix after char c
std::string getSuffix(std::string input, char c);