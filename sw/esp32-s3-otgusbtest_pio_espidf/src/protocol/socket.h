/*
Implementation of the ORacon protocol
*/
#pragma once

#include "protocol_message.h"
#include "protocol_parser.h"
#include "serial_commands.cpp"
#include "si/si_parser.h"
#include "defines.h"
#include <string>
#include <vector>

//Checks if text starts with given prefix
bool startsWith(std::string text, std::string prefix);

// Writes the given raw data to serial
bool writeData(const std::string &data);

// Sends data and checks if they got received correctly
bool sendData(const byte *data, int dataLen, int socketId);

// Sends given protocol message
bool sendMessage(const ProtocolMessage &protocolMessage);

//Receives raw data from the serial port
std::string receiveRawData();

//Receives and parses the raw data - decrypts
std::string getData();

ProtocolMessage getNewMessage();

bool sendAck(DeviceStatus &status);

bool sendNack(DeviceStatus &status);

void connectSocket(DeviceStatus &status);

void authenticateDevice(DeviceStatus &status);

void closeSocket(DeviceStatus &status);

bool sendStatus(DeviceStatus &status, DeviceConfig &config);

bool sendPunches(std::vector<SIRecord> records);

int getSignalStrength();