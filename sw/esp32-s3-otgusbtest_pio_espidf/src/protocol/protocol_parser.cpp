#include "protocol_parser.h"

std::string messageTypeToString(ProtocolMessageType type)
{
    switch (type)
    {
    case ACK:
        return "ACK";
    case NACK:
        return "NACK";
    case CONNECT:
        return "CONNECT";
    case STATUS:
        return "STATUS";
    case PUNCH:
        return "PUNCH";
    case CONF:
        return "CONF";
    default:
        // Unknown type -> exception
        throw std::invalid_argument("Invalid protocol message type");
    }
}

ProtocolMessageType stringToMessageType(const std::string &typeString)
{
    if (typeString == "ACK")
    {
        return ProtocolMessageType::ACK;
    }
    else if (typeString == "NACK")
    {
        return ProtocolMessageType::NACK;
    }
    else if (typeString == "STATUS")
    {
        return ProtocolMessageType::STATUS;
    }
    else if (typeString == "PUNCH")
    {
        return ProtocolMessageType::PUNCH;
    }
    else if (typeString == "CONF")
    {
        return ProtocolMessageType::CONF;
    }
    else
    {
        // Unknown type -> exception
        throw std::invalid_argument("Invalid protocol message type");
    }
}

std::string messageToString(const ProtocolMessage &message)
{
    JsonDocument doc;

    doc["device"] = message.deviceId;
    doc["counter"] = message.counter;
    doc["token"] = message.token;

    // TODO: add data

    std::string output;
    serializeJson(doc, output);

    return output;
}

std::string statusToString(const Status &status, const Config &config)
{
    JsonDocument doc;
    JsonObject configJson = doc.createNestedObject("config");
    configJson["statusDelay"] = config.statusDelay;

    JsonObject statusObj = doc.createNestedObject("status");
    statusObj["battery"] = status.battery;
    statusObj["signal"] = status.signal;
    statusObj["punchesReceived"] = status.punchesReceived;

    std::string output;
    serializeJson(doc, output);
    return output;
}

std::string punchToString(SIRecord record)
{
    JsonDocument doc;
    doc["order"] = record.order;
    doc["stationNumber"] = record.stationNumber;
    doc["cardNumber"] = record.cardNumber;
    doc["time"] = record.time;

    std::string output;
    serializeJson(doc, output);
    return output;
}

ProtocolMessage parseMessage(std::string message)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    std::string stringType = doc["type"];
    std::string token = doc["token"];
    int deviceId = doc["device"];
    int counter = doc["counter"];

    if (error)
    {
        // TODO: log error
        // return nullptr;
    }

    ProtocolMessage msg;
    msg.type = stringToMessageType(&stringType);
    msg.deviceId = deviceId;
    msg.counter = counter;
    msg.token = token;

    return msg;
}

Config dataToConfig(std::string data)
{
    Config config;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    int statusDelay = doc["statusDelay"];

    // Error when parsing
    if (error)
    {
    }

    config.statusDelay = statusDelay;
    return config;
}
