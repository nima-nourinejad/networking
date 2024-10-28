#include "Server.hpp"

Server::Server (int port, std::string const & host, std::string const & errorPage, size_t maxBodySize, std::map<std::string, std::string> const & routes)
    : Socket (port, host, errorPage, maxBodySize, routes), _num_clients (0){};

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
	addEpoll (_socket_fd, MAX_CONNECTIONS);
}

void Server::acceptClient ()
{
	std::cout << "Recieved EPOLLIN event on listening socket" << std::endl;
	if (_num_clients >= MAX_CONNECTIONS)
	{
		std::cout << "Max clients reached" << std::endl;
		return ;
	}
	bool noPendingConnections = false;
	while (noPendingConnections == false)
	{
		std::cout << "It seems there are pending connections" << std::endl;
		ClientConnection temp;
	
		bool available = false;
		int i;
		for (i = 0; (i < MAX_CONNECTIONS && signal_status != SIGINT); ++i)
		{
			if (_clients[i].connected == false && _clients[i].fd == -1 && _clients[i].index == -1 
				&& _clients[i].status == DISCONNECTED)
				{
					available = true;
					break;
				}
		}
		if (available == false)
			throw SocketException ("Failed to find availbe slot. Contridiction wiht check of _num_clients");
		std::cout << "There is empty slot for client so I try accept" << std::endl;
		temp.fd = accept (_socket_fd, NULL, NULL);
		if (temp.fd == -1)
		{
			std::cout << "accept returnning -1" << std::endl;
			
			if (errno != EAGAIN)
				throw SocketException ("Failed to accept client");
			else
			{
				noPendingConnections = true;
				std::cout << "No pending connections anymore" << std::endl;
				break;
			}
		}
		else
		{
				std::cout << "Accepted client " << i + 1 << std::endl;
				_clients[i].fd = temp.fd;
				_clients[i].connected = true;
				_clients[i].index = i;
				++_num_clients;
				_clients[i].status = CONNECTED;
				addEpoll (_clients[i].fd, i);
				_clients[i].lastActivity = getCurrentTime();
		}
	}

}


void Server::closeSocket ()
{
	std::cout << std::endl << "Server is shutting down" << std::endl;
	signal (SIGINT, SIG_DFL);
	closeClientSockets ();
	removeEpoll (_socket_fd);
	if (_fd_epoll != -1)
		close (_fd_epoll);
	if (_socket_fd != -1)
		close (_socket_fd);
}

std::string Server::getRequest (int index) const
{
	return _clients[index].request;
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
		_clients[index].status = DISCONNECTED;
		_clients[index].request.clear();
		_clients[index].response.clear();
		_clients[index].index = -1;
		--_num_clients;
	}
}

void Server::closeClientSockets ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
		closeClientSocket (i);
}


int Server::getNumClients () const
{
	return _num_clients;
}


int Server::waitForEvents()
{
	int n_ready_fds = epoll_wait(_fd_epoll, _ready, MAX_CONNECTIONS + 1, 1000);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			return 0;
		else
			throw SocketException ("Failed to wait for events");
	}
	return n_ready_fds;
}

std::string Server::finfPath(std::string const & method, std::string const & uri) const
{
	std::string path;
	if (method == "GET" && uri == "/")
		path = _config.routes.at("/");
	else if (method == "GET" && uri == "/about")
		path = _config.routes.at("/about");
	else if (method == "GET" && uri == "/delay")
		path = _config.routes.at("/delay");
	else
		path = _config.errorPage;
	return path;
}

void Server::createResponse(int index)
{
	_clients[index].status = PROCESSING;
	std::cout << "Creating response for client " << index + 1 << std::endl;
	std::string method = requestmethod(_clients[index].request);
	std::cout << "Method: " << method << std::endl;
	std::string uri = requestURI(_clients[index].request);
	std::cout << "URI: " << uri << std::endl;
	std::string path = finfPath(method, uri);
	std::cout << "Path: " << path << std::endl;
	std::string body = readFile(path);
	if (method == "GET" && (uri == "/" || uri == "/about"))
		_clients[index].response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
	else
		_clients[index].response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
	_clients[index].status = READYTOSEND;
	std::cout << "Response created for client " << index + 1 << std::endl;
}

void Server::sendMessage (ClientConnection * client)
{
	std::cout << "Sending message to client " << client->index + 1 << std::endl;
	int index = client->index;
	if (_clients[index].connected == false || index >= MAX_CONNECTIONS || index < 0 || signal_status == SIGINT)
		return;
	ssize_t bytes_sent;
	bytes_sent =  send (_clients[index].fd, _clients[index].response.c_str (), _clients[index].response.size (), MSG_DONTWAIT);
	if (bytes_sent > 0)
	{
		std::cout << "Sent message to client: " << index + 1 << std::endl;
		_clients[index].status = CONNECTED;
	}
}

void Server::receiveMessage(ClientConnection * client)
{
	int index = client->index;
	if (_clients[index].connected == false || index >= MAX_CONNECTIONS || index < 0 || signal_status == SIGINT)
		return;
	char buffer[1024] = {};
	ssize_t bytes_received;
	bytes_received = recv (_clients[index].fd, buffer, sizeof (buffer), MSG_DONTWAIT);
	if (bytes_received == 0)
	{
		std::cout << "Client " << index + 1 << " disconnected" << std::endl;
		closeClientSocket (index);
	}
	else
	if (bytes_received > 0)
	{
		std::cout << "Received message from client " << index + 1 << std::endl;
		_clients[index].status = RECEIVED;
		_clients[index].request = buffer;
	}
}

int Server::getClientStatus(struct epoll_event const & event) const
{
	if (event.data.fd == _socket_fd || event.data.ptr == nullptr)
		return -1;
	ClientConnection *target = (ClientConnection *)event.data.ptr;
	return target->status;
}

int Server::getClientIndex(struct epoll_event const & event) const
{
	if (event.data.fd == _socket_fd || event.data.ptr == nullptr)
		return -1;
	ClientConnection *target = (ClientConnection *)event.data.ptr;
	return target->index;
}

void Server::handleEvents()
{
	int n_ready_fds = waitForEvents();
	int index;
	for (int i = 0 ; i < n_ready_fds; i++)
	{
		if (_ready[i].data.fd == _socket_fd)
		{
			if (_ready[i].events & EPOLLIN)
				acceptClient ();
		}
		else
		{
			index = getClientIndex(_ready[i]);
			if (getClientStatus(_ready[i]) == CONNECTED)
			{
				if (getPassedTime(index) > 5)
				{
					std::cout << "Client " << index + 1 << " timed out" << std::endl;
					closeClientSocket (index);
				}
				else
				{
					if (_ready[i].events & EPOLLIN)
						receiveMessage ((ClientConnection *)_events[i].data.ptr);
				}
			}
			else if (getClientStatus(_ready[i]) == RECEIVED)
			{
				createResponse(index);
			}
			else if (getClientStatus(_ready[i]) == READYTOSEND)
			{
				if (_ready[i].events & EPOLLOUT)
					sendMessage ((ClientConnection *)_events[i].data.ptr);
			}
		}
	}
}

void Server::addEpoll(int fd, int index)
{
	_events[index].data.fd = fd;
	if (index == MAX_CONNECTIONS)
		_events[index].events = EPOLLIN;
	else
	{
		_events[index].events = EPOLLIN | EPOLLOUT;
		_events[index].data.ptr = &_clients[index];
	}
	if (epoll_ctl(_fd_epoll, EPOLL_CTL_ADD, fd, _events + index) == -1)
			throw SocketException ("Failed to add listening port to epoll event");
}

std::string Server::requestURI(std::string const & message) const
{
	std::istringstream stream(message);
	std::string method;
	std::string uri;
	stream >> method >> uri;
	return uri;
}

std::string Server::requestmethod(std::string const & message) const
{
	std::istringstream stream(message);
	std::string method;
	stream >> method;
	return method;
}


time_t Server::getPassedTime(int index) const
{
	time_t current_time = getCurrentTime();
	if (current_time == -1)
		throw SocketException ("Failed to get passed time");
	return (difftime(current_time, _clients[index].lastActivity));
}
