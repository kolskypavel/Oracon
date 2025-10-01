#pragma once

#include <Arduino.h>
#include <esp_timer.h>
#include <stdint.h>
#include <string>
#include "defines.h"

// Current status of socket
enum HttpStatus
{
    HTTP_OFF,
    HTTP_INIT
};

// Contains the current status of device
class DeviceStatus
{
public:
    bool init;

    // System information
    uint8_t battery;
    uint8_t signal;
    uint32_t punchesReceived;

    // HTTP related
    HttpStatus httpStatus;

    // Static fields
    std::string deviceKey;

    // Get the current battery level - read from voltage
    void updateBatteryLevel();
};

long getCurrentTime();
