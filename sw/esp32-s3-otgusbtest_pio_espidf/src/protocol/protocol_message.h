#include <string>
#include <stdint.h>
#include <ArduinoJson.h>

// Represents one message sent over Oracon protocol

enum ProtocolMessageType
{
    ACK,
    NACK,
    CONNECT,
    STATUS,
    PUNCH,
    CONF
};

struct ProtocolMessage
{
    ProtocolMessageType type;
    uint16_t deviceId;
    uint16_t counter;
    std::string token;
    JsonDocument data;
    std::string signature;
};