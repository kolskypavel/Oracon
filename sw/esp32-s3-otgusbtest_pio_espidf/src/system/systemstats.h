#pragma once

#include <stdint.h>
#include <string>
#include <wolfssl/wolfcrypt/ecc.h>

enum RunStatus
{
    OK,
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
    // Static fields
    ecc_key key;
    ecc_key serverKey;
    std::string serverIp;
    uint16_t serverPort;
    uint16_t deviceId;

    // System information
    uint8_t battery;
    uint8_t signal;
    uint32_t punchesReceived;
    RunStatus runStatus;

    // Socket related
    bool connected;
    bool authenticated;
    uint8_t socketId;
    std::string token;
    long counter;

    // Device dynamic config
    DeviceConfig config;

    // Get the current battery level - read from voltage
    void updateBatteryLevel();
};
