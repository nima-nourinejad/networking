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
				_clients[i].index = i;
				++_num_clients;
				addEpollEvent (_clients[i].fd, &_events[i], i);
				std::cout << "Accepted client " << i + 1 << std::endl;
				// std::string reply = "Connction successful";
				// send (_clients[i].fd, reply.c_str (), reply.size (), 0);
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
				removeEpollEvent (_clients[i].fd);
				close (_clients[i].fd);
				_clients[i].index = 0;
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
	close (_fd_epoll);
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
			removeEpollEvent (_clients[i].fd);
			_clients[i].index = 0;
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

struct epoll_event * Server::getEvents()
{
	return _events;
}

int Server::howManyEventsShouldbeHandled()
{
	int n_ready_fds = epoll_wait(_fd_epoll, _events, MAX_CONNECTIONS, 1000);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			return 0;
		else
			throw SocketException ("Failed to wait for events", this);
	}
	return n_ready_fds;
}

int Server::howManyToReceive()
{
	int n_ready_fds = epoll_wait(_fd_epoll, _events, MAX_CONNECTIONS, -1);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			n_ready_fds = 0;
		else
			throw SocketException ("Failed to wait for events", this);
	}
	int count = 0;
	for (int i = 0; i < n_ready_fds ; ++i)
	{
		if (_events[i].events & EPOLLIN)
			count++;
	}
	return count;
}

int Server::howManyToSend()
{
	int n_ready_fds = epoll_wait(_fd_epoll, _events, MAX_CONNECTIONS, -1);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			n_ready_fds = 0;
		else
			throw SocketException ("Failed to wait for events", this);
	}
	int count = 0;
	for (int i = 0; i < n_ready_fds ; ++i)
	{
		if (_events[i].events & EPOLLOUT)
			count++;
	}
	return count;
}




void Server::receiveMessage (int index)
{
	if (_clients[index].connected == false)
		return;
	char buffer[1024];
	memset (buffer, 0, sizeof (buffer));
	ssize_t bytes_received;

	if (index < MAX_CONNECTIONS && signal_status != SIGINT)
	{
		bytes_received = recv (_clients[index].fd, buffer, sizeof (buffer), 0);
		if (bytes_received == -1)
		{
			if (!(errno == EAGAIN || errno == EWOULDBLOCK))
				throw SocketException ("Failed to receive message", this);
		}
		if (bytes_received == 0)
		{
			std::cout << "Client " << index + 1 << " disconnected" << std::endl;
			std::cout << "Closing client " << index + 1 << std::endl;
			removeEpollEvent (_clients[index].fd);
			close (_clients[index].fd);
			_clients[index].fd = -1;
			_clients[index].connected = false;
			_clients[index].sent = false;
			_clients[index].received = false;
			--_num_clients;
		}
		else
		{
			std::cout << "Received message from client " << index + 1 << std::endl;
			_clients[index].message = buffer;
			_clients[index].received = true;
			_clients[index].sent = false;
			std::cout << "Message from client " << index + 1 << " : " << _clients[index].message << std::endl;
		}
	}
}

void Server::sendMessage (int index, std::string message)
{
	
	if (_clients[index].connected == false)
		return;
	ssize_t bytes_sent;
	if (index < MAX_CONNECTIONS && signal_status != SIGINT)
	{
		std::string reply = message;
		bytes_sent =  send (_clients[index].fd, reply.c_str (), reply.size (), 0);
		if (bytes_sent == -1)
		{
			if (!(errno == EAGAIN || errno == EWOULDBLOCK))
				throw SocketException ("Failed to send message", this);
		}
		else
		{
			std::cout << "Sent message to client " << index + 1 << std::endl;
			_clients[index].sent = true;
			_clients[index].received = false;
		}
	}
}

void Server::sendMessage (ClientConnection * client)
{
	int index = client->index;
	// std::string reply = "Server: I got this message: " + _clients[index].message;
	// sendMessage (index, reply);
	std::string html_content = "<html><body><h1>Hello from the Server!</h1></body></html>";
	std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(html_content.size()) + "\r\nConnection: close\r\n\r\n" + html_content;
	sendMessage(index, response);

	
}

void Server::receiveMessage(ClientConnection * client)
{
	int index = client->index;
	receiveMessage (index);
}

void Server::handleEvents()
{
	if (signal_status == SIGINT || _num_clients == 0)
		return;
	int n_ready_fds = howManyEventsShouldbeHandled();
	if (n_ready_fds == 0)
		return;
	int i = 0;
	for (i = 0 ; i < n_ready_fds; i++)
	{
		if (_events[i].events & EPOLLIN)
		{
			receiveMessage ((ClientConnection *)_events[i].data.ptr);
			sendMessage ((ClientConnection *)_events[i].data.ptr);
		}
	}
}

void Server::addEpollEvent(int fd, struct epoll_event * event, int index)
{
	event->events = EPOLLIN | EPOLLET;
	event->data.fd = fd;
	event->data.ptr = &_clients[index];
	if (epoll_ctl(_fd_epoll, EPOLL_CTL_ADD, fd, event) == -1)
		throw SocketException ("Failed to add epoll event", this);
}
