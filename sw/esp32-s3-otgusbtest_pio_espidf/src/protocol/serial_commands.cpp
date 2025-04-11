/*
Commands used to communicate with NB-IOT module 
Reference: https://www.waveshare.com/w/upload/c/c2/SIM7020_Series_TCPIP_Application_Note_V1.02.pdf
*/

#include <string>

//Commands
const std::string COMMAND_CREATE_SOCKET = "AT+CSOC=1,1,1";
const std::string COMMAND_CONNECT = "AT+CSOCON=";
const std::string COMMAND_SEND = "AT+CSOSEND=";     //Structure  AT+CSOSEND=<socket_id>,<len>,<data>
const std::string COMMAND_INCOMMING = "+CSONMI:";
const std::string COMMAND_CLOSE = "AT+CSOCL=";
const std::string COMMAND_SIGNAL = "+CSQ";

const std::string COMMAND_RESPONSE_ERROR = "+CSOERR:";
const std::string COMMAND_RESPONSE_OK = "OK";
const std::string COMMAND_RESPONSE_CREATED = "+CSOC:";

//Error codes