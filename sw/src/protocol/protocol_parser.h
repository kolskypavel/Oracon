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
void hexToData(const std::string &in, byte *out);

// Trim leading and ending whitespaces
void trimmString(std::string &text);

// Checks if text starts with given prefix
bool startsWith(const std::string &text, const std::string &prefix);

// Gets suffix after string str
std::string getSuffix(const std::string &input, const std::string &str);

// Gets substring between two strings
std::string getSubstr(const std::string &input,const std::string & start,const std::string & end);

// Parses the values from a command in a format COM:<X>,<Y>
std::pair<int, int> getValuesFromAt(const std::string & command);

/**
 * @brief Attempts to parse the given string to a protocol message
 * @throws std::illegal_argument if the string is not in the valid format
 */
ProtocolMessage parseMessage(const std::string &message);

// Serialize message to string
std::string messageToString(const ProtocolMessage &message);

// Serialize message type to string
std::string messageTypeToString(ProtocolMessageType type);

// Deserialize string to message type
ProtocolMessageType stringToMessageType(const std::string &typeString);

// Generates signature data from status object
std::string generateSignatureData(const DeviceStatus &status);

// Serializes status object to OraCon format
std::string statusToString(const DeviceStatus &status);

// Serializes punch object to OraCon format
std::string punchToString(const SIRecord &record);

// Serializes punches to string
std::string punchesToString(const SIRecord punches[], int size);

// Deserialize signature 
std::string dataToSignature(const std::string &data);

// Deserialize string to config object
DeviceConfig stringToConfig(const std::string &data);

//Returns a cause for given error code - from SIMCom AT manual
const char * getCause(uint8_t errCode);
