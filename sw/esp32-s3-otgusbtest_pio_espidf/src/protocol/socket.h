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

// Writes the given data to serial
bool writeData(const std::string &data, int dataLen);

//Sends data and checks if they got received correctly
bool sendData(const byte *data, int dataLen, int socketId);

//Sends given protocol message
bool sendMessage(const ProtocolMessage &protocolMessage);

std::string receiveData();

ProtocolMessage getNewMessage();

bool sendAck();

bool sendNack();

void connectSocket(DeviceStatus & status);

void authenticateDevice(DeviceStatus & status);

bool sendStatus(DeviceStatus &status, DeviceConfig &config);

bool sendPunches(std::vector<SIRecord> records);

int getSignalStrength();