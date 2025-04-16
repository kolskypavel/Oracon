/*
Commands used to communicate with NB-IOT module
Reference: https://files.waveshare.com/wiki/SIM7028-NB-IoT-HAT/SIM7028%20Series_TCPIP_Application%20Note_V1.04.pdf
*/

#include <string>

// Commands
const std::string COMMAND_CHECK_SIM = "AT+CPIN?";
const std::string COMMAND_CHECK_SIGNAL = "AT+CSQ";
const std::string COMMAND_CHECK_SERVICE = "AT+CGREG?";
const std::string COMMAND_CREATE_SOCKET = "AT+CSOC=";
const std::string COMMAND_CONNECT = "AT+CSOCON=";
const std::string COMMAND_SEND = "AT+CSOSEND="; // Structure  AT+CSOSEND=<socket_id>,<len>,<data>
const std::string COMMAND_CLOSE = "AT+CSOCL=";

const std::string COMMAND_RESPONSE_SIM_OK = "+CPIN:READY";
const std::string COMMAND_RESPONSE_SERVICE = "+CGREG:";
const std::string COMMAND_RESPONSE_INCOMMING_DATA = "+CSONMI:";
const std::string COMMAND_RESPONSE_ERROR = "+CSOERR:";
const std::string COMMAND_RESPONSE_OK = "OK";
const std::string COMMAND_RESPONSE_CREATED = "+CSOC:";
const std::string COMMAND_RESPONSE_SIGNAL = "+CSQ:";

// Error codes