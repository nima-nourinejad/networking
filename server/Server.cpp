#include "Server.hpp"

Server::Server (int port)
    : Socket (port), _num_clients (0){};

void Server::connectToSocket ()
{
	if (bind (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
		throw std::runtime_error ("Failed to bind socket");
	if (listen (_socket_fd, MAX_CONNECTIONS) == -1)
		throw std::runtime_error ("Failed to listen on socket");
}

void Server::acceptClient ()
{
	_client_fd[_num_clients] = accept (_socket_fd, NULL, NULL);
	if (_client_fd[_num_clients] == -1)
		throw std::runtime_error ("Failed to accept client");
	++_num_clients;
}

void Server::sendMessage (std::string message, int index)
{
	send (_client_fd[index], message.c_str (), message.size (), 0);
}
void Server::receiveMessage (int index)
{
	char buffer[1024];
	memset (buffer, 0, sizeof (buffer));

	recv (_client_fd[index], buffer, sizeof (buffer), 0);
	_message[index] = buffer;
}



void Server::closeSocket ()
{
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
