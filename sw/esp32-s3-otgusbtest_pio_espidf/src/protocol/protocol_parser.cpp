#include "protocol_parser.h"

std::string dataToHex(const byte *data, int dataLen)
{
    std::ostringstream oss;
    for (int i = 0; i < dataLen; ++i)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

void hexToData(std::string in, byte *out)
{
    if (in.length() % 2 != 0)
    {
        throw std::invalid_argument("Invalid input string length or insufficient output buffer size");
    }

    for (size_t i = 0; i < in.length(); i += 2)
    {
        std::string byteString = in.substr(i, 2);
        out[i / 2] = static_cast<byte>(std::stoi(byteString, nullptr, 16));
    }
}

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

std::string statusToString(const DeviceStatus &status)
{
    JsonDocument doc;
    JsonObject configJson = doc.createNestedObject("config");
    configJson["statusDelay"] = status.config.statusDelay;

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

std::string punchesToString(const SIRecord punches[], int size)
{
    JsonDocument doc;
    JsonArray punchesArray = doc["punches"].to<JsonArray>();

    for (int i = 0; i < size; ++i)
    {
        JsonObject punchObj = punchesArray.createNestedObject();
        punchObj["order"] = punches[i].order;
        punchObj["stationNumber"] = punches[i].stationNumber;
        punchObj["cardNumber"] = punches[i].cardNumber;
        punchObj["time"] = punches[i].time;
    }

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
        throw std::invalid_argument("Failed to parse message");
    }

    ProtocolMessage msg;
    msg.type = stringToMessageType(stringType);
    msg.deviceId = deviceId;
    msg.counter = counter;
    msg.token = token;

    return msg;
}

DeviceConfig stringToConfig(const std::string &data)
{
    DeviceConfig config;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    int statusDelay = doc["statusDelay"];

    // Error when parsing
    if (error)
    {
        throw std::invalid_argument("Invalid configuration format");
    }

    config.statusDelay = statusDelay;
    return config;
}

std::string generateSignatureData(const DeviceStatus &status)
{
    std::string sigData = std::to_string(status.deviceId);
    byte out[100]; // TODO: modify
    word32 outLen;

    generateSignature(sigData, status.key, out, outLen);
    JsonDocument doc;
    doc["signature"] = dataToHex(out, outLen);

    std::string json;
    serializeJson(doc, json);

    return json;
}

bool startsWith(std::string text, std::string prefix)
{
    if (text.rfind(prefix, 0) == 0)
    {
        return true;
    }
    return false;
}

std::string getSuffix(std::string input, char c)
{
    return input.substr(input.find(c) + 1);
}
