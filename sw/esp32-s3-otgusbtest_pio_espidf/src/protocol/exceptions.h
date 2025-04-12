#pragma once

#include <stdexcept>

class SocketException : public std::runtime_error
{
public:
    SocketException(const std::string &cause) : std::runtime_error(cause) {}
};

class SerialException : public std::runtime_error
{
public:
    SerialException(const std::string &cause) : std::runtime_error(cause) {}
};