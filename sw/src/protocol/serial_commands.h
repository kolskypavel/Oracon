/*
Commands used to communicate with NB-IOT module
Reference: https://files.waveshare.com/wiki/SIM7028-NB-IoT-HAT/SIM7028%20Series_TCPIP_Application%20Note_V1.04.pdf
*/

#include <string>

// Commands
const std::string COMMAND_CHECK_AT = "AT";
const std::string COMMAND_CHECK_SIM = "AT+CPIN?";
const std::string COMMAND_CHECK_SIGNAL = "AT+CSQ";
const std::string COMMAND_CHECK_SERVICE = "AT+CEREG?";
const std::string COMMAND_RECEIVE_DATA = "AT+CIPRXGET=";
const std::string COMMAND_SET_TIMEOUT = "AT+CIPTIMEOUT=";
const std::string COMMAND_CREATE_SOCKET = "AT+NETOPEN";
const std::string COMMAND_CONNECT = "AT+CIPOPEN="; // Structure AT+CIPOPEN=<link_num>,"TCP",<serverIP>,<serverPort>
const std::string COMMAND_SEND = "AT+CIPSEND="; // Structure  AT+CIPSEND=<link_num>
const std::string COMMAND_CLOSE = "AT+NETCLOSE";

const std::string COMMAND_RESPONSE_SIM_OK = "+CPIN: READY";
const std::string COMMAND_RESPONSE_SERVICE = "+CEREG:";
const std::string COMMAND_RESPONSE_SOCKET_EXISTING = "+NETOPEN: 1";
const std::string COMMAND_RESPONSE_CONNECTED = "+CIPOPEN:";
const std::string COMMAND_RESPONSE_SEND = ">";
const std::string COMMAND_RESPONSE_SEND_ERROR = "+CIPERROR: ";
const std::string COMMAND_RESPONSE_INCOMMING_DATA = "+CIPRXGET";
const std::string COMMAND_RESPONSE_ERROR = "ERROR:";
const std::string COMMAND_RESPONSE_CLOSE_SOCKET = "+IPCLOSE:";
const std::string COMMAND_RESPONSE_OK = "OK";
const std::string COMMAND_RESPONSE_SIGNAL = "+CSQ:";

// Error codes