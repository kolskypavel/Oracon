#pragma once

#include <esp_timer.h>
#include <stdint.h>
#include <string>
#include "wolfssl.h"
#include <wolfssl/wolfcrypt/ecc.h>
#include "defines.h"

// Current status of socket
enum SocketStatus
{
    SOCKET_OFF,
    SOCKET_CONNECTED,
    SOCKET_AUTHENTICATED
};

// Used for LEDs
enum RunStatus
{
    STATUS_OK,
    INIT_ERROR,
    SERIAL_ERROR,
    SOCKET_ERROR
};

// Contains the dynamic configuration of device
struct DeviceConfig
{
    uint8_t statusDelay;
};

// Contains the current status of device
class DeviceStatus
{
public:
    // System information
    uint8_t battery;
    uint8_t signal;
    uint32_t punchesReceived;
    RunStatus runStatus;

    // Socket related
    SocketStatus socketStatus;
    uint8_t socketId;
    std::string token;

    // Device dynamic config
    DeviceConfig config;

    // Static fields
    uint16_t deviceId;
    ecc_key* key;
    ecc_key* publicKey;
    ecc_key* serverKey;

    // Get the current battery level - read from voltage
    void updateBatteryLevel();
};

long getCurrentTime();
