#include "Server.hpp"

Server::Server (int port)
    : Socket (port, "Server"), _num_clients (0){
		std::cout << "Server is starting" << std::endl;
		for (int i = 0; i < MAX_CONNECTIONS; ++i)
			_client_fd[i] = -1;
	};

void Server::connectToSocket ()
{
	std::cout << "Server is connecting" << std::endl;
	if (signal_status != SIGINT)
	{
		if (bind (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
			throw SocketException ("Failed to bind socket", this);
		std::cout << _name << " is binded on port " << _port << std::endl;
	}
	if (signal_status != SIGINT)
	{
		if (listen (_socket_fd, MAX_CONNECTIONS) == -1)
			throw SocketException ("Failed to listen on socket", this);
	}
	std::cout << _name << " is listening on port " << _port << std::endl;
}

void Server::acceptClient ()
{
	if (_num_clients == MAX_CONNECTIONS)
	{
		std::cout << "Server is full" << std::endl;
		return;
	}
	if (_num_clients < MAX_CONNECTIONS)
		std::cout << "Server is accepting clients" << std::endl;
	for (int i = 0; (i < _num_clients && signal_status != SIGINT); ++i)
	{
		if (_client_fd[i] == -1)
		{
			_client_fd[i] = accept (_socket_fd, NULL, NULL);
			if (_client_fd[i] == -1)
			{
				if (!acceptableError (errno))
					throw SocketException ("Failed to accept client", this);
				else{
					std::cout << "connection in progress" << std::endl;
					break;
				}
			}
			else
			{
				std::cout << "Client " << i << " connected" << std::endl;
				++_num_clients;
			}
		}
	}
}

void Server::sendMessage (std::string message)
{
	if (_num_clients == 0)
	{
		std::cout << "No clients connected to send message" << std::endl;
		return;
	}
	std::cout << "Server is sending message" << std::endl;
	ssize_t bytes_sent;
	for (int i = 0; (i < _num_clients && signal_status != SIGINT); ++i)
	{
		if (_client_fd[i] != -1)
		{
			std::string reply = "Server: I got this message from you: " + _message[i] + ". My reply is: " + message;
			bytes_sent =  send (_client_fd[i], reply.c_str (), reply.size (), 0);
			if (bytes_sent == -1)
			{
				if (!acceptableError (errno))
					throw SocketException ("Failed to send message", this);
			}
		}
	}
}
void Server::receiveMessage ()
{
	if (_num_clients == 0)
	{
		std::cout << "No clients connected to receive message" << std::endl;
		return;
	}
	std::cout << "Server is receiving message" << std::endl;


	char buffer[1024];
	ssize_t bytes_received;

	for (int i = 0; (i < _num_clients && signal_status != SIGINT); ++i)
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
			{
				std::cout << "Received message from client " << i << std::endl;
				_message[i] = buffer;
			}
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
