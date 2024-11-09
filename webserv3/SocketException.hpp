#ifndef SOCKETEXCEPTION_HPP
#define SOCKETEXCEPTION_HPP

#include <stdexcept>
#include <string>
#include <cstring>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

class SocketException : public std::runtime_error
{
	  public:
	SocketException (std::string const & message);
	void log() const;
};

#endif