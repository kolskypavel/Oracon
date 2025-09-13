/*
Commands used to communicate with NB-IOT module
Reference: https://files.waveshare.com/wiki/SIM7028-NB-IoT-HAT/SIM7028%20NB-IoT%20HAT-Doc/SIM7028_Series_HTTP(S)_Application_Note_V1.04.pdf
*/

#include <string>

// Commands
const std::string COMMAND_CHECK_AT = "AT";
const std::string COMMAND_CHECK_SIM = "AT+CPIN?";
const std::string COMMAND_CHECK_SIGNAL = "AT+CSQ";
const std::string COMMAND_CHECK_SERVICE = "AT+CEREG?";

const std::string COMMAND_HTTP_INIT = "AT+HTTPINIT";
const std::string COMMAND_HTTP_SET_PARAMETERS = "AT+HTTPPARA=";
const std::string COMMAND_HTTP_SSL_CONFIG = "SSLCFG";
const std::string COMMAND_HTTP_URL = "SSLCFG";
const std::string COMMAND_HTTP_DATA = "AT+HTTPDATA=";
const std::string COMMAND_HTTP_ACTION = "AT+HTTPACTION";
const std::string COMMAND_HTTP_READ_RESPONSE = "AT+HTTPHEAD";

// SSL related commands


// Command responses
const std::string COMMAND_RESPONSE_SIM_OK = "+CPIN: READY";
const std::string COMMAND_RESPONSE_SERVICE = "+CEREG:";
const std::string COMMAND_RESPONSE_SEND_ERROR = "+CIPERROR: ";
const std::string COMMAND_RESPONSE_INCOMMING_DATA = "+CIPRXGET";
const std::string COMMAND_RESPONSE_ERROR = "ERROR:";
const std::string COMMAND_RESPONSE_OK = "OK";
const std::string COMMAND_RESPONSE_SIGNAL = "+CSQ:";