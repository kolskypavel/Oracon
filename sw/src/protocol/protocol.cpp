#include "protocol.h"

void writeData(const std::string &data)
{
    nbiot_serial.println(data.c_str());

#ifdef TEST_ORACON_SERIAL_VERBOSE
    ESP_LOGI("SOCKET", "Wrote data: %s", data.c_str());
#endif

    delay(100);
}

void clearInBuffer()
{
    delay(2000);
    while (nbiot_serial.available())
    {
        nbiot_serial.read();
    }
}

std::string receiveRawData(uint8_t timeout)
{
    long i = 0;
    int curr = 0;
    std::string out = "";

    while (nbiot_serial.available() == 0)
    {
        if (curr >= timeout)
        {
            throw SocketException("Socket read timeout expired");
        }

#ifdef TEST_ORACON_SERIAL_VERBOSE
        ESP_LOGI("SOCKET", "TIMEOUT %d", curr);
#endif

        curr++;
        delay(1000);
    }

    while (nbiot_serial.available())
    {
#ifdef LIMIT_NB_IOT_SERIAL
        if (i >= MAX_MESSAGE_SIZE)
        {
            // Clear incoming buffer
            while (Serial.available())
            {
                Serial.read();
            }

            throw std::invalid_argument("Message length exceeded maximal size");
        }
#endif

        char c = nbiot_serial.read();
        out += c;
        i++;
    }

#ifdef TEST_ORACON_SERIAL_VERBOSE
    ESP_LOGI("DATA", "Received %ld B: %s", i, out.c_str());
#endif

    trimmString(out); // Trim leading/trailing whitespaces
    return out;
}

bool initSsl()
{
    std::string resp;

    writeData(COMMAND_SSL_LIST_CERT);

    resp = receiveRawData(SOCKET_READ_TIMEOUT);

    // Check if module contains the server certificate
    if (!contains(resp, SERVER_CERTIFICATE_NAME))
    {
        writeData(COMMAND_SSL_UPLOAD_CERT + "\"" + SERVER_CERTIFICATE_NAME + "\"," + std::to_string(strlen(SERVER_CERTIFICATE)));
        writeData(SERVER_CERTIFICATE);

        if (!startsWith(resp, COMMAND_RESPONSE_OK))
        {
            ESP_LOGE("SSL", "Failed to upload SSL certificate");
            return false;
        }

        writeData(COMMAND_SSL_LIST_CERT);
        resp = receiveRawData(SOCKET_READ_TIMEOUT);

        if (!contains(resp, SERVER_CERTIFICATE_NAME))
        {
            ESP_LOGE("SSL", "Failed to list uploaded SSL certificate");
            return false;
        }
    }

    writeData(COMMAND_SSL_CONFIG + COMMAND_SSL_CONFIG_CERT + SERVER_CERTIFICATE_NAME);
    resp = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(resp, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("SSL", "Failed to set SSL config");
        return false;
    }

    return true;
}

void initHttp(DeviceStatus &status)
{
    std::string resp;

    // Check SIM
    writeData(COMMAND_CHECK_SIM);
    resp = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(resp, COMMAND_RESPONSE_SIM_OK))
    {
        throw SocketException("SIM not connected");
    }

    getSignalStrength(status);
    ESP_LOGI("CONNECT", "Signal strength: %d dBm", status.signal);

    if (status.signal == MAX_SIGNAL_VALUE)
    {
        ESP_LOGE("CONNECT", "No signal");
        return;
    }

    // Check service status
    writeData(COMMAND_CHECK_SERVICE);
    resp = receiveRawData(SOCKET_READ_TIMEOUT);
    if (!startsWith(resp, COMMAND_RESPONSE_SERVICE))
    {
        ESP_LOGE("CONNECT", "Invalid response for service command");
        return;
    }

    std::pair value = getValuesFromAt(resp);

    if (value.second != 1 && value.second != 5)
    {
        ESP_LOGE("CONNECT", "Failed to register to service, code %d", value.second);
        return;
    }

    // Set socket to init HTTP
    writeData(COMMAND_HTTP_INIT);
    resp = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(resp, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("INIT", "Failed to init HTTP");
        return;
    }

    // Set the url address
    writeData(COMMAND_HTTP_SET_PARAMETERS + COMMAND_HTTP_URL + SERVER_ADDRESS + "\"");
    resp = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(resp, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("INIT", "Failed to set URL");
        return;
    }

    if (!initSsl())
    {
        ESP_LOGE("INIT", "Failed to init SSL");
        return;
    }

    ESP_LOGI("CONNECT", "Socket init successful");
    status.httpStatus = HttpStatus::HTTP_INIT;
}

bool sendHttpData(const std::string data, DeviceStatus &status)
{
    std::string buffer;

    // Set content type
    writeData(COMMAND_HTTP_SET_PARAMETERS + COMMAND_HTTP_CONTENT + COMMAND_HTTP_CONTENT_JSON);
    buffer = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(buffer, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("INIT", "Failed to set content type");
        return false;
    }

    // Add data
    writeData(COMMAND_HTTP_SET_PARAMETERS + COMMAND_HTTP_TIMEOUT + std::to_string(HTTP_TIMEOUT));
    buffer = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(buffer, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("INIT", "Failed to set timeout");
        return false;
    }

    writeData(COMMAND_HTTP_DATA + std::to_string(data.size()) + ",10");

    buffer = receiveRawData(SOCKET_READ_TIMEOUT);
    if (!contains(buffer, COMMAND_RESPONSE_HTTP_DOWNLOAD))
    {
        ESP_LOGE("HTTP", "Failed to set HTTP data size");
        return false;
    }

    writeData(data);
    buffer = receiveRawData(SOCKET_READ_TIMEOUT);

    if (!startsWith(buffer, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("HTTP", "Failed to set HTTP data");
        return false;
    }

    // Send the request
    writeData(COMMAND_HTTP_ACTION + std::to_string(HTTP_POST));

    buffer = receiveRawData(SOCKET_READ_TIMEOUT);
    if (!startsWith(buffer, COMMAND_RESPONSE_OK))
    {
        ESP_LOGE("HTTP", "Failed to send HTTP request");
        return false;
    }

    buffer = receiveRawData(SOCKET_HTTP_TIMEOUT);

    // Get the response code
    writeData(COMMAND_HTTP_READ_HEAD);
    buffer = receiveRawData(SOCKET_READ_TIMEOUT);

    int replyStatus = getStatusFromHttpHead(buffer);

    // writeData(COMMAND_HTTP_READ_RESPONSE + "0,500");
    // buffer = receiveRawData();

    // if (!startsWith(buffer, COMMAND_RESPONSE_OK))
    // {
    //     ESP_LOGE("HTTP", "Failed to get HTTP response");
    //     return false;
    // }

    switch (replyStatus)
    {
    case HTTP_STATUS_OK:
    {
        ESP_LOGI("HTTP", "Data sent successfully");
        return true;
    }
    break;
    case HTTP_STATUS_BAD_REQUEST:
    case HTTP_STATUS_NOT_FOUND:
    {
        ESP_LOGE("HTTP", "Incorrect / missing API key");
        return false;
    }
    break;

    case HTTP_STATUS_DATA_WRONG:
    {
        writeData(COMMAND_HTTP_READ_RESPONSE + "200");
        buffer = receiveRawData(SOCKET_READ_TIMEOUT);

        return false;
    }

    default:
    {
        ESP_LOGE("HTTP", "Unknown reply status");
        break;
    }
    }
    return false;
}

void sendStatus(DeviceStatus &status, Preferences &prefs)
{

    std::string msg = statusToString(status);
    ESP_LOGI("STATUS", "Status data: %s", msg.c_str());

    bool sent = sendHttpData(msg, status);

    if (sent)
    {
        ESP_LOGI("STATUS", "Status succesfully received by server");
    }
    else
    {
        ESP_LOGE("STATUS", "Status not received by server");
    }
}

bool sendPunches(DeviceStatus &status, SIRecord punches[], int punchCount)
{
    std::string msg = punchesToString(punches, punchCount, status);

    ESP_LOGI("PUNCH", "Sending %d punches", punchCount);
    bool sent = sendHttpData(msg, status);

    if (sent)
    {
        ESP_LOGI("PUNCH", "Punches succesfully received by server");
        return true;
    }
    else
    {
        ESP_LOGE("PUNCH", "Punches not received by server");
        return false;
    }
}

void getSignalStrength(DeviceStatus &status)
{
    writeData(COMMAND_CHECK_SIGNAL);
    std::string response = receiveRawData(SOCKET_READ_TIMEOUT); // Format +CSQ: <rssi>,<ber>

    if (!startsWith(response, COMMAND_RESPONSE_SIGNAL))
    {
        throw std::invalid_argument("Invalid signal strength response");
    }

    std::pair values = getValuesFromAt(response);
    int rssi = values.first;

    // NB-Iot signal not detectable
    if (rssi == 99)
    {
        status.signal = MAX_SIGNAL_VALUE;
    }
    // Convert RSSI to dBm using TS 27.007 Section 8.5
    else if (rssi >= 0 && rssi <= 31)
    {
        status.signal = MAX_SIGNAL_VALUE - (rssi * 2); // dBm calculation
    }
}