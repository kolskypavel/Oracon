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

void hexToData(const std::string &in, byte *out)
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

void trimmString(std::string &text)
{
    text.erase(text.find_last_not_of(WHITESPACES) + 1);
    text.erase(0, text.find_first_not_of(WHITESPACES));
}

bool startsWith(const std::string &text, const std::string &prefix)
{
    if (text.compare(0, prefix.size(), prefix) == 0)
    {
        return true;
    }
    return false;
}

bool contains(const std::string &input, const std::string &data)
{
    return input.find(data) != std::string::npos;
}

std::string getSuffix(const std::string &input, const std::string &str)
{
    size_t pos = input.rfind(str);
    if (pos != std::string::npos)
    {
        return input.substr(pos + str.length());
    }
    else
    {
        throw std::invalid_argument("Substring not found in input string");
    }
}

std::string getSubstr(const std::string &input, const std::string &start, const std::string &end)
{
    size_t startPos = input.find(start);
    size_t endPos = input.find(end);
    if (startPos != std::string::npos && endPos != std::string::npos)
    {
        return input.substr(startPos + start.size(), endPos - startPos);
    }
    else
    {
        throw std::invalid_argument("Substring not found in input string");
    }
}

std::pair<int, int> getValuesFromAt(const std::string &command)
{
    std::string trimmed = getSuffix(command, ": ");
    size_t commaPos = trimmed.find(',');
    if (commaPos != std::string::npos)
    {
        std::string firstStr = trimmed.substr(0, commaPos);
        std::string secondStr = trimmed.substr(commaPos + 1);

        int first = std::stoi(firstStr);
        int second = std::stoi(secondStr);

        return std::make_pair(first, second);
    }

    throw std::invalid_argument("Invalid command format - can't extract values");
}

std::string statusToString(const DeviceStatus &status)
{
    JsonDocument doc;
    doc["type"] = "STATUS";
    doc["device_key"] = DEVICE_API_KEY;
    doc["battery"] = status.battery;
    doc["signal"] = status.signal;
    
    std::string output;
    serializeJson(doc, output);
    return output;
}

std::string punchesToString(const SIRecord punches[], int size, const DeviceStatus &status)
{
    JsonDocument doc;
    doc["type"] = "PUNCH";
    doc["device_key"] = DEVICE_API_KEY;
    JsonArray punchesArray = doc["results"].to<JsonArray>();

    for (int i = 0; i < size; ++i)
    {
        ESP_LOGI("PARSER", "Punch: [S %d,C %d, T %s]",
                 punches[i].stationNumber,
                 punches[i].cardNumber,
                 punches[i].time);

        JsonObject punchObj = punchesArray.add<JsonObject>();
        punchObj["control_code"] = punches[i].stationNumber;
        punchObj["si_number"] = punches[i].cardNumber;
        punchObj["punch_time"] = punches[i].time;
    }

    std::string output;
    serializeJson(doc, output);
    return output;
}

int getStatusFromHttpHead(const std::string &head)
{
    // Find the start of the HTTP status line
    size_t httpPos = head.find("HTTP/");
    if (httpPos == std::string::npos)
    {
        return -1;
    }
    // Find the end of the line
    size_t lineEnd = head.find('\n', httpPos);
    size_t lineStart = httpPos;
    std::string line = (lineEnd != std::string::npos) ? head.substr(lineStart, lineEnd - lineStart) : head.substr(lineStart);

    // Find first and second space
    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        return -1;
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
        return -1;

    // Extract status code
    int status = 0;
    for (size_t i = firstSpace + 1; i < secondSpace; ++i)
    {
        char c = line[i];
        if (c < '0' || c > '9')
            return -1;
        status = status * 10 + (c - '0');
    }
    return status;
}

const char *getCause(uint8_t errCode)
{
    switch (errCode)
    {
    case 0:
        return "Operation succeeded";
    case 1:
        return "Network failure";
    case 2:
        return "Network not opened";
    case 3:
        return "Wrong parameter";
    case 4:
        return "Operation not supported";
    case 5:
        return "Failed to create socket";
    case 6:
        return "Failed to bind socket";
    case 7:
        return "TCP server is already listening";
    case 8:
        return "Busy";
    case 9:
        return "Sockets opened";
    case 10:
        return "Timeout";
    case 11:
        return "DNS parse failed for AT+CIPOPEN";
    case 12:
        return "Unknown error";
    default:
        return "Unrecognized error code";
    }
}