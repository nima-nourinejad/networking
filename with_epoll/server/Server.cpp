#include "Server.hpp"

Server::Server (int port)
    : Socket (port, "Server"), _num_clients (0){};

void Server::connectToSocket ()
{
	if (signal_status != SIGINT)
	{
		if (bind (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
			throw SocketException ("Failed to bind socket", this);
	}
	if (signal_status != SIGINT)
	{
		if (listen (_socket_fd, BACKLOG) == -1)
			throw SocketException ("Failed to listen on socket", this);
	}
	std::cout << _name << " is listening on port " << _port << std::endl;
}

void Server::acceptClient ()
{
	if (_num_clients >= MAX_CONNECTIONS)
		return ;
	for (int i = 0; (i < MAX_CONNECTIONS && signal_status != SIGINT); ++i)
	{
		if (_clients[i].connected == false)
		{
			_clients[i].fd = accept (_socket_fd, NULL, NULL);
			if (_clients[i].fd == -1)
			{
				
				if (!acceptableError (errno))
					throw SocketException ("Failed to accept client", this);
				else
					break;
			}
			else
			{
				_clients[i].connected = true;
				++_num_clients;
				std::string reply = "Connction successful";
				send (_clients[i].fd, reply.c_str (), reply.size (), 0);
			}
		}
	}
}

void Server::sendMessage (std::string message)
{
	if (_num_clients == 0)
		return;
	ssize_t bytes_sent;
	for (int i = 0; (i < MAX_CONNECTIONS && signal_status != SIGINT); ++i)
	{
		if (_clients[i].connected == true && _clients[i].received == true && _clients[i].sent == false)
		{
			std::string reply = message;
			bytes_sent =  send (_clients[i].fd, reply.c_str (), reply.size (), 0);
			if (bytes_sent == -1)
			{
				if (!acceptableError (errno))
					throw SocketException ("Failed to send message", this);
			}
			else
			{
				std::cout << "Sent message to client " << i + 1 << std::endl;
				_clients[i].sent = true;
				_clients[i].received = false;
			}
		}
	}
}


void Server::receiveMessage ()
{
	if (_num_clients == 0)
		return;

	char buffer[1024];
	ssize_t bytes_received;

	for (int i = 0; (i < MAX_CONNECTIONS && signal_status != SIGINT); ++i)
	{
		if (_clients[i].connected == true && _clients[i].received == false)
		{
			memset (buffer, 0, sizeof (buffer));

			bytes_received = recv (_clients[i].fd, buffer, sizeof (buffer), 0);
			if (bytes_received == -1)
			{
				if (!acceptableError (errno))
					throw SocketException ("Failed to receive message", this);
			}
			if (bytes_received == 0)
			{
				std::cout << "Client " << i + 1 << " disconnected" << std::endl;
				std::cout << "Closing client " << i + 1 << std::endl;
				close (_clients[i].fd);
				_clients[i].fd = -1;
				_clients[i].connected = false;
				_clients[i].sent = false;
				_clients[i].received = false;
				--_num_clients;
			}
			else
			{
				std::cout << "Received message from client " << i + 1 << std::endl;
				_clients[i].message = buffer;
				_clients[i].received = true;
				_clients[i].sent = false;
				std::cout << "Message from client " << i + 1 << " : " << _clients[i].message << std::endl;
			}
		}
	}
}

void Server::closeSocket ()
{
	std::cout << std::endl << _name << " is shutting down" << std::endl;
	signal (SIGINT, SIG_DFL);
	closeClientSockets ();
	close (_socket_fd);
}

std::string Server::getMessage (int index) const
{
	return _clients[index].message;
}

void Server::closeClientSockets ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].connected == true)
		{
			std::cout << "Closing client " << i + 1 << std::endl;
			close (_clients[i].fd);
			_num_clients--;
			_clients[i].fd = -1;
			_clients[i].connected = false;
			_clients[i].sent = false;
			_clients[i].received = false;
		}
		
	}
}

int Server::getClientFD (int index) const
{
	return _clients[index].fd;
}

int Server::getNumClients () const
{
	return _num_clients;
}

int Server::getMaxConnections () const
{
	return MAX_CONNECTIONS;
}

int Server::numRecievedMessages() const
{
	int count = 0;
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].received == true)
			count++;
	}
	return count;
}

int Server::numSentMessages() const
{
	int count = 0;
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].sent == true)
			count++;
	}
	return count;
}
