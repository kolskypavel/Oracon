#pragma once

#include <esp_timer.h>
#include <stdint.h>
#include <string>
#include "wolfssl.h"
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/aes.h>
#include "defines.h"

// Current status of socket
enum SocketStatus
{
    SOCKET_OFF,
    SOCKET_CONNECTED,
    SOCKET_AUTHENTICATED
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
    bool init;

    // System information
    uint8_t battery;
    uint8_t signal;
    uint32_t punchesReceived;

    // Socket related
    SocketStatus socketStatus;
    uint8_t socketId;

    // Device dynamic config
    DeviceConfig config;

    // Static fields
    uint16_t deviceId;
    uint8_t aesKey[AES_KEY_SIZE];
    RsaKey *privateKey;
    RsaKey *publicKey;
    RsaKey *serverKey;

    // Get the current battery level - read from voltage
    void updateBatteryLevel();
};

long getCurrentTime();
