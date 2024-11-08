#include "SocketException.hpp"

SocketException::SocketException (std::string const & message)
    : std::runtime_error (message + " : " + strerror (errno)){};