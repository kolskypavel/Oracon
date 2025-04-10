#pragma once

#include <stdint.h>
#include <string>

// Contains the current status of device
struct DeviceStatus
{
    bool connected;
    bool authenticated;
    uint8_t socketId;
    std::string token;
    uint8_t battery;
    uint8_t signal;
    uint16_t punchesReceived;
};

// Contains the dynamic configuration of device
struct DeviceConfig
{
    int statusDelay;
};

int getBatteryStatus();
