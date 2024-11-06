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
		return;
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
			if (_clients[i].status == DISCONNECTED && _clients[i].fd == -1 && _clients[i].index == -1)
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
			std::cout << "Accepted client " << i + 1 << ". Waiting for the rquest" << std::endl;
			_clients[i].fd = temp.fd;
			_clients[i].index = i;
			++_num_clients;
			_clients[i].status = CONNECTED;
			addEpoll (_clients[i].fd, i);
			_clients[i].connectTime = getCurrentTime ();
		}
	}
}

void Server::closeSocket ()
{
	std::cout << std::endl
		  << "Server is shutting down" << std::endl;
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
	if (_clients[index].fd != -1 && index < MAX_CONNECTIONS && index >= 0)
	{
		std::cout << "Closing client " << index + 1 << std::endl;
		removeEpoll (_clients[index].fd);
		close (_clients[index].fd);
		_clients[index].fd = -1;
		_clients[index].status = DISCONNECTED;
		_clients[index].chunkedRecive = false;
		_clients[index].keepAlive = true;
		_clients[index].connectTime = 0;
		_clients[index].request.clear ();
		_clients[index].response.clear ();
		_clients[index].responseParts.clear ();
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

int Server::waitForEvents ()
{
	int n_ready_fds = epoll_wait (_fd_epoll, _ready, MAX_CONNECTIONS + 1, -1);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			return 0;
		else
			throw SocketException ("Failed to wait for events");
	}
	return n_ready_fds;
}

std::string Server::findPath (std::string const & method, std::string const & uri) const
{
	std::string path;
	if (method == "GET" && uri == "/")
		path = _config.routes.at ("/");
	else if (method == "GET" && uri == "/about")
		path = _config.routes.at ("/about");
	else if (method == "GET" && uri == "/long")
		path = _config.routes.at ("/long");
	else
		path = _config.errorPage;
	return path;
}

std::string Server::createStatusLine (std::string const & method, std::string const & uri) const
{
	std::string statusLine;
	if (method == "GET" && (uri == "/" || uri == "/about" || uri == "/long"))
		statusLine = "HTTP/1.1 200 OK\r\n";
	else
		statusLine = "HTTP/1.1 404 Not Found\r\n";
	return statusLine;
}

void Server::connectionType (int index)
{
	if (_clients[index].request.find ("Connection: close") != std::string::npos)
		_clients[index].keepAlive = false;
}

void Server::createResponseParts (int index)
{
	_clients[index].status = PROCESSING;
	connectionType (index);
	std::cout << "Creating response for client " << index + 1 << std::endl;
	std::string method = requestmethod (_clients[index].request);
	std::cout << "Method: " << method << std::endl;
	std::string uri = requestURI (_clients[index].request);
	std::cout << "URI: " << uri << std::endl;
	std::string path = findPath (method, uri);
	std::cout << "Path: " << path << std::endl;
	std::string body = readFile (path);

	std::string statusLine = createStatusLine (method, uri);

	std::string contentType = "Content-Type: text/html\r\n";
	std::string connection;
	if (_clients[index].keepAlive)
		connection = "Connection: keep-alive\r\n";
	else
		connection = "Connection: close\r\n";
	
	std::string header;
	if (body.size() > _config.maxBodySize)
	{
		std::string transferEncoding = "Transfer-Encoding: chunked\r\n";
		header = statusLine + contentType + transferEncoding + connection;
		_clients[index].responseParts.push_back (header + "\r\n");
		size_t chunkSize;
		std::string chunk;
		std::stringstream sstream;
		while (body.size () > 0)
		{
			chunkSize = std::min (body.size (), _config.maxBodySize);
			chunk = body.substr (0, chunkSize);
			sstream.str ("");
			sstream << std::hex << chunkSize << "\r\n";
			sstream << chunk << "\r\n";
			_clients[index].responseParts.push_back (sstream.str ());
			body = body.substr (chunkSize);
		}
		_clients[index].responseParts.push_back ("0\r\n\r\n");
	}
	else
	{
		std::string contentLength = "Content-Length: " + std::to_string (body.size ()) + "\r\n";
		header = statusLine + contentType + contentLength + connection;
		_clients[index].responseParts.push_back (header + "\r\n" + body);
	}
	_clients[index].status = READYTOSEND;
	std::cout << "Response created for client " << index + 1 << std::endl;
}

void Server::sendResponseParts (ClientConnection * client)
{
	int index = client->index;
	if (_clients[index].fd == -1 || index >= MAX_CONNECTIONS || index < 0 || signal_status == SIGINT)
		return;
	ssize_t bytes_sent;
	bytes_sent = send (_clients[index].fd, _clients[index].responseParts[0].c_str (), _clients[index].responseParts[0].size (), MSG_DONTWAIT);
	if (bytes_sent == 0)
	{
		std::cout << "Client " << index + 1 << " disconnected" << std::endl;
		closeClientSocket (index);
		return;
	}
	else if (bytes_sent > 0)
	{
		if (bytes_sent < static_cast<ssize_t>(_clients[index].responseParts[0].size()))
		{
			std::string remainPart = _clients[index].responseParts[0].substr(bytes_sent);
			_clients[index].responseParts[0] = remainPart;
			return;
		}
		else
		{
			_clients[index].responseParts.erase (_clients[index].responseParts.begin ());
			if (_clients[index].responseParts.empty ())
			{
				std::cout << "All response parts sent to client " << index + 1 << ". Waiting for the new request" << std::endl;
				if (_clients[index].keepAlive == false)
					closeClientSocket (index);
				else
				{
					_clients[index].request.clear ();
					_clients[index].response.clear ();
					_clients[index].status = CONNECTED;
					_clients[index].connectTime = getCurrentTime ();
				}
			}
		}
	}
}

void Server::changeRequestToNotFound (int index)
{
	_clients[index].request.clear ();
	_clients[index].request = "Get /notfound HTTP/1.1\r\n";
	_clients[index].status = RECEIVED;
	_clients[index].chunkedRecive = false;
}

void Server::grabChunkedHeader (std::string & unProcessed, std::string & header, int index)
{
	header = unProcessed.substr (0, unProcessed.find ("\r\n\r\n") + 4);
	unProcessed = unProcessed.substr (unProcessed.find ("\r\n\r\n") + 4);
	_clients[index].request = header;
}

size_t Server::getChunkedSize (std::string & unProcessed, int index)
{
	size_t chunkedSize;
	std::string sizeString;
	sizeString = unProcessed.substr (0, unProcessed.find ("\r\n"));
	unProcessed = unProcessed.substr (unProcessed.find ("\r\n") + 2);
	try
	{
		chunkedSize = std::stoul (sizeString, nullptr, 16);
	}
	catch (...)
	{
		changeRequestToNotFound (index);
		return 0;
	}
	if (unProcessed.size () < chunkedSize + 2)
	{
		changeRequestToNotFound (index);
		return 0;
	}
	return chunkedSize;
}

void Server::grabChunkedData (std::string & unProcessed, size_t chunkedSize, int index)
{
	std::string data;
	data = unProcessed.substr (0, chunkedSize);
	_clients[index].request += data;
	unProcessed = unProcessed.substr (chunkedSize + 2);
}

void Server::handleChunkedEncoding (int index)
{
	std::string unProcessed = _clients[index].request;
	_clients[index].request.clear ();
	if (unProcessed.find ("Transfer-Encoding: chunked") == std::string::npos || unProcessed.find ("\r\n\r\n") == std::string::npos)
		return (changeRequestToNotFound (index));
	std::string header = "";
	grabChunkedHeader (unProcessed, header, index);

	size_t chunkedSize;
	while (true)
	{
		if (unProcessed.find ("\r\n") == std::string::npos)
			return (changeRequestToNotFound (index));
		chunkedSize = getChunkedSize (unProcessed, index);
		if (chunkedSize == 0)
		{
			_clients[index].status = RECEIVED;
			_clients[index].chunkedRecive = false;
			return;
		}
		else
			grabChunkedData (unProcessed, chunkedSize, index);
	}
}

bool Server::finishedReceiving (int index)
{
	size_t contentLength;
	std::string contentLengthString;
	if (_clients[index].request.find ("Content-Length: ") == std::string::npos)
	{
		std::cout << "No content length found" << std::endl;
		return true;
	}
	contentLengthString = _clients[index].request.substr (_clients[index].request.find ("Content-Length: ") + 16);
	if (contentLengthString.find ("\r\n") == std::string::npos)
	{
		std::cerr << "Failed to find end of content length" << std::endl;
		changeRequestToNotFound (index);
		return true;
	}
	contentLengthString = contentLengthString.substr (0, contentLengthString.find ("\r\n"));
	try
	{
		contentLength = std::stoul (contentLengthString);
	}
	catch (...)
	{
		std::cerr << "Failed to convert content length to number" << std::endl;
		changeRequestToNotFound (index);
		return true;
	}
	if (receivedLength (index) > contentLength)
	{
		std::cerr << "Received more data than expected" << std::endl;
		changeRequestToNotFound (index);
		return true;
	}
	if (receivedLength (index) == contentLength)
		return true;
	return false;
}

size_t Server::receivedLength (int index) const
{
	size_t headerLength = _clients[index].request.find ("\r\n\r\n") + 4;
	size_t receivedLength = _clients[index].request.size () - headerLength;
	return receivedLength;
}

void Server::receiveMessage (ClientConnection * client)
{
	int index = client->index;
	if (_clients[index].fd == -1 || index >= MAX_CONNECTIONS || index < 0 || signal_status == SIGINT)
		return;
	char buffer[16384] = {};
	std::string stringBuffer;
	ssize_t bytes_received;
	bytes_received = recv (_clients[index].fd, buffer, sizeof (buffer), MSG_DONTWAIT);
	if (bytes_received == 0)
	{
		std::cout << "Client " << index + 1 << " disconnected" << std::endl;
		closeClientSocket (index);
	}
	else if (bytes_received > 0)
	{
		std::cout << "Received message from client " << index + 1 << std::endl;
		stringBuffer = buffer;
		_clients[index].request += stringBuffer;
		if (_clients[index].chunkedRecive == false)
		{
			if (_clients[index].request.find ("\r\n\r\n") == std::string::npos)
			{
				if (_clients[index].request.size () > MAX_HEADER_SIZE)
				{
					std::cout << "Header size exceeded the limit" << std::endl;
					changeRequestToNotFound (index);
				}
				return;
			}
			else
			{
				if (_clients[index].request.find ("Transfer-Encoding: chunked") != std::string::npos)
					_clients[index].chunkedRecive = true;
				else
				{
					if (finishedReceiving (index))
						_clients[index].status = RECEIVED;
				}
			}
		}
		else
		{
			if (_clients[index].request.find ("\r\n0\r\n\r\n") != std::string::npos)
				handleChunkedEncoding (index);
		}
	}
}

int Server::getClientStatus (struct epoll_event const & event) const
{
	if (event.data.fd == _socket_fd || event.data.ptr == nullptr)
		return -1;
	ClientConnection * target = (ClientConnection *)event.data.ptr;
	return target->status;
}

int Server::getClientIndex (struct epoll_event const & event) const
{
	if (event.data.fd == _socket_fd || event.data.ptr == nullptr)
		return -1;
	ClientConnection * target = (ClientConnection *)event.data.ptr;
	return target->index;
}

void Server::handleTimeouts (int index)
{
	std::cout << "Client " << index + 1 << " timed out" << std::endl;
	if (_clients[index].request.empty () == false)
	{
		_clients[index].status = RECEIVED;
		createResponseParts (index);
	}
	else
		closeClientSocket (index);
}

void Server::handleEvents ()
{
	int n_ready_fds = waitForEvents ();
	int index;
	for (int i = 0; i < n_ready_fds; i++)
	{
		if (_ready[i].data.fd == _socket_fd)
		{
			if (_ready[i].events & (EPOLLHUP | EPOLLERR))
			{
				std::cerr << "Error on listening socket" << std::endl;
				removeEpoll (_socket_fd);
				close(_socket_fd);
				createSocket ();
				makeSocketReusable ();
				setAddress ();
				connectToSocket ();
				addEpoll (_socket_fd, MAX_CONNECTIONS);
			}
			else if (_ready[i].events & EPOLLIN)
				acceptClient ();
		}
		else
		{
			index = getClientIndex (_ready[i]);
			if (_ready[i].events & (EPOLLHUP | EPOLLERR))
			{
				std::cout << "Client " << index + 1 << " disconnected" << std::endl;
				closeClientSocket (index);
			}
			else
			{
				if (getClientStatus (_ready[i]) == CONNECTED)
				{
					if (getPassedTime (index) > TIMEOUT)
					{
						handleTimeouts (index);
					}
					else
					{
						if (_ready[i].events & EPOLLIN)
							receiveMessage ((ClientConnection *)_events[i].data.ptr);
					}
				}
				else if (getClientStatus (_ready[i]) == RECEIVED)
				{
					createResponseParts (index);
				}
				else if (getClientStatus (_ready[i]) == READYTOSEND)
				{
					if (_ready[i].events & EPOLLOUT)
						sendResponseParts ((ClientConnection *)_events[i].data.ptr);
				}
			}
		}
	}
}

void Server::addEpoll (int fd, int index)
{
	_events[index].data.fd = fd;
	if (index == MAX_CONNECTIONS)
		_events[index].events = EPOLLIN | EPOLLHUP | EPOLLERR;
	else
	{
		_events[index].events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
		_events[index].data.ptr = &_clients[index];
	}
	if (epoll_ctl (_fd_epoll, EPOLL_CTL_ADD, fd, _events + index) == -1)
		throw SocketException ("Failed to add listening port to epoll event");
}

std::string Server::requestURI (std::string const & message) const
{
	std::istringstream stream (message);
	std::string method;
	std::string uri;
	stream >> method >> uri;
	return uri;
}

std::string Server::requestmethod (std::string const & message) const
{
	std::istringstream stream (message);
	std::string method;
	stream >> method;
	return method;
}

time_t Server::getPassedTime (int index) const
{
	time_t current_time = getCurrentTime ();
	if (current_time == -1)
		throw SocketException ("Failed to get passed time");
	return (difftime (current_time, _clients[index].connectTime));
}
