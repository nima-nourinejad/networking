#include "Client.hpp"

Client::Client (int port)
    : Socket (port)
{
	_client_fd[0] = _socket_fd;
	_client_fd[1] = _socket_fd;
};
void Client::connectToSocket ()
{
	if (connect (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
		throw std::runtime_error ("Failed to connect to socket");
};
