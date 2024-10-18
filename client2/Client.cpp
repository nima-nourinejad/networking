#include "Client.hpp"

Client::Client (int port)
    : Socket (port){};
void Client::connectToSocket ()
{
	if (connect (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
		throw std::runtime_error ("Failed to connect to socket");
};

void Client::closeSocket ()
{
	close (_socket_fd);
};

void Client::sendMessage (std::string message)
{
	send (_socket_fd, message.c_str (), message.size (), 0);
};

void Client::receiveMessage ()
{
	char buffer[1024];
	memset (buffer, 0, sizeof (buffer));

	recv (_socket_fd, buffer, sizeof (buffer), 0);
	_message = buffer;
};

std::string Client::getMessage () const
{
	return _message;
};
