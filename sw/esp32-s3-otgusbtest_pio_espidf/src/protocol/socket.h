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

// Writes the given raw data to serial
bool writeData(const std::string &data);

// Sends data and checks if they got received correctly
bool sendData(const byte *data, int dataLen, int socketId);

// Receives raw data from the serial port
std::string receiveRawData();

// Receives and parses the raw data - converts and decrypts
std::string getData();

// Sends given protocol message
bool sendMessage(const ProtocolMessage &protocolMessage, DeviceStatus &status);

// Validates the given message
bool validateMessage(const ProtocolMessage &msg, DeviceStatus &status);

// Gets a new message from the server
ProtocolMessage getNewMessage(DeviceStatus &status);

// Init message with data from status struct
void initMessage(ProtocolMessage &msg, DeviceStatus &status);

bool sendAck(DeviceStatus &status);

bool sendNack(DeviceStatus &status);

void connectSocket(DeviceStatus &status);

void authenticateDevice(DeviceStatus &status);

void closeSocket(DeviceStatus &status);

bool sendStatus(DeviceStatus &status);

bool sendPunches(DeviceStatus &status, SIRecord punches[], int punchCount);

// Get the current signal strength
void getSignalStrength(DeviceStatus &status);