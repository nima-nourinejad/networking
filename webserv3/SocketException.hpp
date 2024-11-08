#ifndef SOCKETEXCEPTION_HPP
#define SOCKETEXCEPTION_HPP

#include <stdexcept>
#include <string>
#include <cstring>

class SocketException : public std::runtime_error
{
	  public:
	SocketException (std::string const & message);
};

#endif