#include "protocol_parser.h"

std::string messageTypeToString(ProtocolMessageType type)
{
    switch (type)
    {
    case TYPE_ACK:
        return "ACK";
    case TYPE_NACK:
        return "NACK";
    case TYPE_CONNECT:
        return "CONNECT";
    case TYPE_STATUS:
        return "STATUS";
    case TYPE_PUNCH:
        return "PUNCH";
    case TYPE_CONF:
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
        return ProtocolMessageType::TYPE_ACK;
    }
    else if (typeString == "NACK")
    {
        return ProtocolMessageType::TYPE_NACK;
    }
    else if (typeString == "STATUS")
    {
        return ProtocolMessageType::TYPE_STATUS;
    }
    else if (typeString == "PUNCH")
    {
        return ProtocolMessageType::TYPE_PUNCH;
    }
    else if (typeString == "CONF")
    {
        return ProtocolMessageType::TYPE_CONF;
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

std::string statusToString(const DeviceStatus &status, const DeviceConfig &config)
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

std::string punchToString(const SIRecord &record)
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

ProtocolMessage parseMessage(const std::string &message)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    std::string stringType = doc["type"];
    std::string data = doc["data"];
    std::string token = doc["token"];
    int deviceId = doc["device"];
    int counter = doc["counter"];

    if (error)
    {
        // TODO: log error
        // return nullptr;
    }

    ProtocolMessage msg;
    msg.type = stringToMessageType(stringType);
    msg.deviceId = deviceId;
    msg.counter = counter;
    msg.token = token;

    return msg;
}

DeviceConfig dataToConfig(const std::string &data)
{
    DeviceConfig config;
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