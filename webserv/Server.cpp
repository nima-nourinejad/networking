#include "Server.hpp"

Server::Server (int port, std::string const & host)
    : Socket (port, host), _num_clients (0){};

void Server::connectToSocket ()
{
	if (signal_status != SIGINT)
	{
		if (bind (_socket_fd, (struct sockaddr *)&_address, sizeof (_address)) == -1)
			throw SocketException ("Failed to bind socket");
	}
	if (signal_status != SIGINT)
	{
		if (listen (_socket_fd, BACKLOG) == -1)
			throw SocketException ("Failed to listen on socket");
	}
	std::cout << "Server is listening on host " << _config.host << " and port " << _config.port << std::endl;
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
				
				if (errno != EAGAIN)
					throw SocketException ("Failed to accept client");
				else
					break;
			}
			else
			{
				_clients[i].connected = true;
				_clients[i].index = i;
				++_num_clients;
				addEpoll (_clients[i].fd, &_events[i], i);
				std::cout << "Accepted client " << i + 1 << std::endl;
			}
		}
	}
}


void Server::closeSocket ()
{
	std::cout << std::endl << "Server is shutting down" << std::endl;
	signal (SIGINT, SIG_DFL);
	closeClientSockets ();
	if (_fd_epoll != -1)
		close (_fd_epoll);
	if (_socket_fd != -1)
		close (_socket_fd);
}

std::string Server::getMessage (int index) const
{
	return _clients[index].message;
}

void Server::closeClientSocket (int index)
{
	if (_clients[index].connected == true && index < MAX_CONNECTIONS && index >= 0)
	{
		std::cout << "Closing client " << index + 1 << std::endl;
		removeEpoll (_clients[index].fd);
		close (_clients[index].fd);
		_clients[index].fd = -1;
		_clients[index].connected = false;
		_clients[index].message = "";
		_clients[index].index = -1;
		--_num_clients;
	}
}

void Server::closeClientSockets ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
		closeClientSocket (i);
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


struct epoll_event * Server::getEvents()
{
	return _events;
}

int Server::waitForEvents()
{
	int n_ready_fds = epoll_wait(_fd_epoll, _events, MAX_CONNECTIONS, 1000);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			return 0;
		else
			throw SocketException ("Failed to wait for events");
	}
	return n_ready_fds;
}


void Server::sendMessage (ClientConnection * client)
{
	int index = client->index;
	std::string html_content = "<html><body><h1>Hello from the Server!</h1></body></html>";
	std::string message = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(html_content.size()) + "\r\nConnection: close\r\n\r\n" + html_content;
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
				throw SocketException ("Failed to send message");
		}
		else
			std::cout << "Sent message to client " << index + 1 << std::endl;
	}	
}

void Server::receiveMessage(ClientConnection * client)
{
	int index = client->index;
	if (_clients[index].connected == false)
		return;
	char buffer[1024] = {};
	ssize_t bytes_received;

	if (index < MAX_CONNECTIONS && signal_status != SIGINT)
	{
		bytes_received = recv (_clients[index].fd, buffer, sizeof (buffer), 0);
		if (bytes_received == -1)
		{
			if (!(errno == EAGAIN || errno == EWOULDBLOCK))
				throw SocketException ("Failed to receive message");
		}
		if (bytes_received == 0)
		{
			std::cout << "Client " << index + 1 << " disconnected" << std::endl;
			closeClientSocket (index);
		}
		else
		{
			std::cout << "Received message from client " << index + 1 << std::endl;
			_clients[index].message = buffer;
			std::cout << "Message from client " << index + 1 << " : " << _clients[index].message << std::endl;
		}
	}
}

void Server::handleEvents()
{
	if (signal_status == SIGINT || _num_clients == 0)
		return;
	int n_ready_fds = waitForEvents();
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

void Server::addEpoll(int fd, struct epoll_event * event, int index)
{
	event->events = EPOLLIN | EPOLLET;
	event->data.fd = fd;
	event->data.ptr = &_clients[index];
	if (epoll_ctl(_fd_epoll, EPOLL_CTL_ADD, fd, event) == -1)
		throw SocketException ("Failed to add epoll event");
}
