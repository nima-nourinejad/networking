#include "Socket.hpp"

Socket::Socket (int port)
    : _port (port)
{
	createSocket ();
	setAddress ();
}

void Socket::createSocket ()
{
	_socket_fd = socket (AF_INET, SOCK_STREAM, 0);
	if (_socket_fd == -1)
		throw std::runtime_error ("Failed to create socket");
}

void Socket::setAddress ()
{
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons (_port);
}

int Socket::getSocketFD () const
{
	return _socket_fd;
}
