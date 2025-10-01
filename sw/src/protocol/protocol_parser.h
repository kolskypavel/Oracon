// Parser class for the protocol messages
#pragma once

#include <ArduinoJson.h>

#include "system/systemstats.h"
#include "si/si_parser.h"
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

// Serializes status object to OraCon format
std::string statusToString(const DeviceStatus &status);

// Serializes punches to string
std::string punchesToString(const SIRecord punches[], int size, const DeviceStatus &status);

// Gets a status from HTTP response
int getStatusFromHttpHead(const std::string & head);

//Returns a cause for given error code - from SIMCom AT manual
const char * getCause(uint8_t errCode);
