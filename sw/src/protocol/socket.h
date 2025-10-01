/*
Implementation of the ORacon protocol
*/
#pragma once

#include <Preferences.h>
#include <string>
#include <vector>

#include "protocol_parser.h"
#include "serial_commands.h"
#include "si/si_parser.h"
#include "exceptions.h"
#include "defines.h"
#include "secrets.h"

// Writes the given raw data to serial
void writeData(const std::string &data);

// Clears the serial buffer in case of exception
void clearInBuffer();

// Sends data and checks if they got received correctly
bool sendHttpData(const std::string data, DeviceStatus &status);

// Receives raw data from the serial port
std::string receiveRawData();

// Receives and parses the raw data - converts and decrypts
std::string getData(DeviceStatus &status);

// Inits the socket for connection
void initHttp(DeviceStatus &status);

// Sends the current device status
void sendStatus(DeviceStatus &status, Preferences &prefs);

// Sends the punches to the server, returns true for OK, false if the punches did not send
bool sendPunches(DeviceStatus &status, SIRecord punches[], int punchCount);

// Get the current signal strength
void getSignalStrength(DeviceStatus &status);