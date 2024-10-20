#include "Client.hpp"

Client::Client (int port, std::string const & name)
    : Socket (port, name), _connected (false){
		std::cout << _name << " is starting" << std::endl;
	};
void Client::connectToSocket ()
{
	if (!_connected)
		std::cout << _name << " is connecting" << std::endl;
	if (!_connected && signal_status != SIGINT)
	{
		if (connect (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
		{
			if (errno != EINPROGRESS)
				throw SocketException ("Failed to connect to socket", this);
		}
		else
		{
			_connected = true;
			std::cout << _name << " is connected" << std::endl;
		}
	}
};

void Client::closeSocket ()
{
	std::cout << _name << " is shutting down" << std::endl;
	signal (SIGINT, SIG_DFL);
	close (_socket_fd);
};

void Client::sendMessage (std::string message)
{
	if (_connected)
		std::cout << _name << " is sending message" << std::endl;
	if (_connected && signal_status != SIGINT)
		send (_socket_fd, message.c_str (), message.size (), 0);
};

void Client::receiveMessage ()
{
	if (_connected)
		std::cout << _name << " is receiving message" << std::endl;
	char buffer[1024];
	memset (buffer, 0, sizeof (buffer));
	ssize_t bytes_received;

	if (_connected && signal_status != SIGINT)
	{
		bytes_received = recv (_socket_fd, buffer, sizeof (buffer), 0);
		if (bytes_received == -1)
		{
			if (!acceptableError (errno))
				throw SocketException ("Failed to receive message", this);
		}
		else if (bytes_received == 0)
		{
			_connected = false;
			std::cout << "Connection closed by server" << std::endl;
		}
		else
			_message = buffer;
	}
};

std::string Client::getMessage () const
{
	return _message;
};

bool Client::isConnected () const
{
	return _connected;
};
