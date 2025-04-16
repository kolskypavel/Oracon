/*
Commands used to communicate with NB-IOT module
Reference: https://files.waveshare.com/wiki/SIM7028-NB-IoT-HAT/SIM7028%20Series_TCPIP_Application%20Note_V1.04.pdf
*/

#include <string>

// Commands
const std::string COMMAND_CHECK_AT = "AT";
const std::string COMMAND_CHECK_SIM = "AT+CPIN?";
const std::string COMMAND_CHECK_SIGNAL = "AT+CSQ";
const std::string COMMAND_CHECK_SERVICE = "AT+CREG?";
const std::string COMMAND_CREATE_SOCKET = "AT+NETOPEN";
const std::string COMMAND_CONNECT = "AT+CIPOPEN="; // Structure AT+CIPOPEN=<link_num>,"TCP",<serverIP>,<serverPort>
const std::string COMMAND_SEND = "AT+CIPSEND="; // Structure  AT+CIPSEND=<link_num>
const std::string COMMAND_CLOSE = "AT+NETCLOSE";

const std::string COMMAND_RESPONSE_SIM_OK = "+CPIN:READY";
const std::string COMMAND_RESPONSE_SERVICE = "+CGREG:";
const std::string COMMAND_RESPONSE_SEND = ">";
const std::string COMMAND_RESPONSE_INCOMMING_DATA = "+RECV FROM:";
const std::string COMMAND_RESPONSE_ERROR = "ERROR:";
const std::string COMMAND_RESPONSE_OK = "OK";
const std::string COMMAND_RESPONSE_CREATED = "+CSOC:";
const std::string COMMAND_RESPONSE_SIGNAL = "+CSQ:";

// Error codes