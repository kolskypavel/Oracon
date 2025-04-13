#pragma once

#include <stdint.h>
#include <string>
#include <wolfssl/wolfcrypt/ecc.h>

// Contains the dynamic configuration of device
struct DeviceConfig
{
    int statusDelay;
};

// Contains the current status of device
class DeviceStatus
{
public:
    // Static fields
    ecc_key key;
    ecc_key serverKey;
    std::string serverIp;
    uint8_t serverPort;
    uint8_t deviceId;

    // System information
    uint8_t battery;
    uint8_t signal;
    uint16_t punchesReceived;

    // Socket related
    bool connected;
    bool authenticated;
    uint8_t socketId;
    std::string token;
    uint16_t counter;

    // Device dynamic config
    DeviceConfig config;

    // Get the current battery level - read from voltage
    void updateBatteryLevel();
};
