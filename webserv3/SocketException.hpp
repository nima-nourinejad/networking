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

enum ErrorType
{
	FIND_EMPTY_SLOT,
	ADD_EPOLL,
	ACCEPT_CLIENT,
};



class SocketException : public std::runtime_error
{
	  public:
	SocketException (std::string const & message);
	SocketException (std::string const & message, int open_fd);
	void log() const;
	int type;
	int open_fd;
};

#endif