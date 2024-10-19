#include "Server.hpp"

Server::Server (int port)
    : Socket (port, "Server"), _num_clients (0){};

void Server::connectToSocket ()
{
	if (bind (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
		throw SocketException ("Failed to bind socket", this);
	if (listen (_socket_fd, MAX_CONNECTIONS) == -1)
		throw SocketException ("Failed to listen on socket", this);
}

void Server::acceptClient ()
{
	int possibleClient = accept (_socket_fd, NULL, NULL);
	if (possibleClient == -1)
	{
		if (!acceptableError (errno))
			throw SocketException ("Failed to accept client", this);
	}
	else
	{
		_client_fd[_num_clients] = possibleClient;
		++_num_clients;
	}
}

void Server::sendMessage (std::string message)
{
	for (int i = 0; i < _num_clients; ++i)
	{
		if (_client_fd[i] != -1)
		{
			std::string reply = "Server: I got this message from you: " + _message[i] + ". My reply is: " + message;
			send (_client_fd[i], reply.c_str (), reply.size (), 0);
		}
	}
}
void Server::receiveMessage ()
{
	char buffer[1024];
	ssize_t bytes_received;

	for (int i = 0; i < _num_clients; ++i)
	{
		if (_client_fd[i] != -1)
		{
			memset (buffer, 0, sizeof (buffer));

			bytes_received = recv (_client_fd[i], buffer, sizeof (buffer), 0);
			if (bytes_received == -1)
			{
				if (!acceptableError (errno))
					throw SocketException ("Failed to receive message", this);
			}
			else if (bytes_received == 0)
			{
				std::cout << "Client " << i << " disconnected" << std::endl;
				close (_client_fd[i]);
				_client_fd[i] = -1;
			}
			else
				_message[i] = buffer;
		}
	}
}

void Server::closeSocket ()
{
	std::cout << _name << " is shutting down" << std::endl;
	signal (SIGINT, SIG_DFL);
	closeClientSockets ();
	close (_socket_fd);
}

std::string Server::getMessage (int index) const
{
	return _message[index];
}

void Server::closeClientSockets ()
{
	for (int i = 0; i < _num_clients; ++i)
	{
		if (_client_fd[i] != -1)
		{
			close (_client_fd[i]);
			_client_fd[i] = -1;
		}
	}
	_num_clients = 0;
}

int Server::getClientFD (int index) const
{
	return _client_fd[index];
}

int Server::getNumClients () const
{
	return _num_clients;
}

int Server::getMaxConnections () const
{
	return MAX_CONNECTIONS;
}


