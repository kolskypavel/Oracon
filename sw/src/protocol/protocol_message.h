#include <string>
#include <stdint.h>
#include <ArduinoJson.h>

// Represents one message sent over Oracon protocol
#pragma once

enum ProtocolMessageType
{
    TYPE_ACK,
    TYPE_NACK,
    TYPE_CONNECT,
    TYPE_STATUS,
    TYPE_PUNCH,
    TYPE_CONF
};

struct ProtocolMessage
{
    ProtocolMessageType type;
    uint16_t deviceId;
    std::string token;
    std::string data;
    std::string signature;
};