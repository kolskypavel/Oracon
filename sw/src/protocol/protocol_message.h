#include <string>
#include <stdint.h>
#include <ArduinoJson.h>

// Represents one message sent over Oracon protocol
#pragma once

enum ProtocolMessageType
{
    TYPE_STATUS,
    TYPE_PUNCH
};

struct ProtocolMessage
{
    ProtocolMessageType type;
    uint16_t deviceId;
    std::string token;
    std::string data;
};