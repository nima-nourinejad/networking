#include "Server.hpp"

Server::Server (int port, std::string const & host, std::map<std::string, std::string> routes)
    : Socket (port, host, std::move(routes)), _num_clients (0){};

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
	addEpoll (_socket_fd, &_events[MAX_CONNECTIONS], MAX_CONNECTIONS);
}

void Server::acceptClient ()
{
	if (_num_clients >= MAX_CONNECTIONS)
		return ;
	int i;
	for (i = 0; (i < MAX_CONNECTIONS && signal_status != SIGINT); ++i)
	{
		if (_clients[i].connected == false && _clients[i].fd == -1 && _clients[i].index == -1 
			&& _clients[i].status == DISCONNECTED)
			break;
	}
	_clients[i].fd = accept (_socket_fd, NULL, NULL);
	if (_clients[i].fd == -1)
	{
		
		if (errno != EAGAIN)
			throw SocketException ("Failed to accept client");
	}
	else
	{
		_clients[i].connected = true;
		_clients[i].index = i;
		++_num_clients;
		addEpoll (_clients[i].fd, &_events[i], i);
		_clients[i].status = CONNECTED;
		std::cout << "Accepted client " << i + 1 << std::endl;
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
		_clients[index].status = DISCONNECTED;
		_clients[index].message.clear();
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
	int n_ready_fds = epoll_wait(_fd_epoll, _ready, MAX_CONNECTIONS + 1, 5000);
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
		path = _config.routes.at("404");
	return path;
}


void Server::createResponse(int index)
{
	std::string method = requestmethod(_clients[index].message);
	std::string uri = requestURI(_clients[index].message);
	std::string path = finfPath(method, uri);
	std::string body = readFile(path);
	if (method == "GET" && (uri == "/" || uri == "/about"))
		_clients[index].response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
	// else if (method == "GET" && uri == "/delay")
	// {
	// 	std::this_thread::sleep_for(std::chrono::seconds(10));
	// 	_clients[index].response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: keep-alive\r\n\r\n" + body;
	// 	_clients[index].status = READYTOSEND;
	// 	modifyEpoll (index, EPOLLOUT);
	// }
	else
		_clients[index].response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
	_clients[index].status = READYTOSEND;
	modifyEpoll (index, EPOLLOUT);
	
}

void Server::sendMessage (ClientConnection * client)
{
	int index = client->index;
	if (_clients[index].connected == false || index >= MAX_CONNECTIONS || index < 0 || signal_status == SIGINT)
		return;
	ssize_t bytes_sent;
	bytes_sent =  send (_clients[index].fd, _clients[index].response.c_str (), _clients[index].response.size (), 0);
	if (bytes_sent > 0)
	{
		std::cout << "Sent message to client " << index + 1 << std::endl;
		closeClientSocket (index);
		// _clients[index].status = CONNECTED;
		// modifyEpoll (index, EPOLLIN);
	}
	

}

void Server::receiveMessage(ClientConnection * client)
{
	int index = client->index;
	if (_clients[index].connected == false || index >= MAX_CONNECTIONS || index < 0 || signal_status == SIGINT)
		return;
	char buffer[1024] = {};
	ssize_t bytes_received;
	bytes_received = recv (_clients[index].fd, buffer, sizeof (buffer), 0);
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
		modifyEpoll (index, 0);
		_clients[index].message = buffer;
		_clients[index].status = PROCESSING;
		createResponse(index);
	}
}

int Server::getClientStatus(struct epoll_event const & event) const
{
	if (event.data.ptr == nullptr)
		return -1;
	return ((ClientConnection *)event.data.ptr)->status;
}

void Server::handleEvents()
{
	if (signal_status == SIGINT)
		return;
	int n_ready_fds = waitForEvents();
	if (n_ready_fds == 0)
		return;
	for (int i = 0 ; i < n_ready_fds; i++)
	{
		if ((_ready[i].events & EPOLLIN) && _ready[i].data.fd == _socket_fd)
				acceptClient ();
		else if ((_ready[i].events & EPOLLIN) && (getClientStatus(_ready[i]) == CONNECTED))
			receiveMessage ((ClientConnection *)_events[i].data.ptr);
		else if ((_ready[i].events & EPOLLOUT) && (getClientStatus(_ready[i]) == READYTOSEND))
			sendMessage ((ClientConnection *)_events[i].data.ptr);
	}
}

void Server::addEpoll(int fd, struct epoll_event * event, int index)
{
	if (index == MAX_CONNECTIONS)
	{
		event->events = EPOLLIN;
		event->data.fd = fd;
		if (epoll_ctl(_fd_epoll, EPOLL_CTL_ADD, fd, event) == -1)
			throw SocketException ("Failed to add listening port to epoll event");
	}
	else
	{
		event->events = EPOLLIN;
		event->data.fd = fd;
		event->data.ptr = &_clients[index];
		if (epoll_ctl(_fd_epoll, EPOLL_CTL_ADD, fd, event) == -1)
			throw SocketException ("Failed to add epoll event");
	}
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

void Server::modifyEpoll(int index, uint32_t status)
{
	_events[index].events = status;
	if (epoll_ctl(_fd_epoll, EPOLL_CTL_MOD, _clients[index].fd, &_events[index]) == -1)
		throw SocketException ("Failed to modify epoll event");
}

void Server::showActiveClients()
{
	std::cout << "Active clients: " << std::endl;
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].connected == true)
		{	std::cout << "Client " << i + 1 << std::endl;
			std::cout << "Message: " << _clients[i].message << std::endl;
			std::cout << "Response: " << _clients[i].response << std::endl;
			std::cout << "FD: " << _clients[i].fd << std::endl;
		}
	}
}
