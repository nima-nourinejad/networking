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
		throw SocketException ("Failed to create socket", nullptr);
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

void Socket::makeSocketNonBlocking ()
{
	int currentFlags = fcntl (_socket_fd, F_GETFL, 0);
	if (currentFlags == -1)
		throw SocketException ("Failed to get socket flags", this);
	int newFlags = currentFlags | O_NONBLOCK;
	if (fcntl (_socket_fd, F_SETFL, newFlags) == -1)
		throw SocketException ("Failed to set socket flags", this);
}
