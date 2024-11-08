#include "Server.hpp"

Server::Server (int port, std::string const & host, size_t maxBodySize)
    : _socket_fd (-1), _fd_epoll (-1), _config (port, host, maxBodySize), _num_clients (0)
{
	applyCustomSignal ();
	createSocket ();
	makeSocketReusable ();
	setAddress ();
	createEpoll ();
};

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
	std::cout << "There are pending connections" << std::endl;
	if (_num_clients >= MAX_CONNECTIONS)
	{
		std::cout << "Max clients reached" << std::endl;
		return;
	}
	bool noPendingConnections = false;
	while (noPendingConnections == false)
	{
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
		temp.fd = accept (_socket_fd, NULL, NULL);
		if (temp.fd == -1)
		{
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
			_clients[i].status = WAITFORREQUEST;
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
		_clients[index].keepAlive = true;
		_clients[index].connectTime = 0;
		_clients[index].request.clear ();
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
	int n_ready_fds = epoll_wait (_fd_epoll, _ready, MAX_CONNECTIONS + 1, 0);
	if (n_ready_fds == -1)
	{
		if (errno == EINTR)
			return 0;
		else
			throw SocketException ("Failed to wait for events");
	}
	return n_ready_fds;
}

std::string findPath (std::string const & method, std::string const & uri)
{
	std::string path;
	if (method == "GET" && uri == "/")
		path = "html/index.html";
	else if (method == "GET" && uri == "/about")
		path = "html/about.html";
	else if (method == "GET" && uri == "/long")
		path = "html/long.html";
	else if (method == "GET" && uri == "/400")
		path = "html/400.html";
	else if (method == "GET" && uri == "/500")
		path = "html/500.html";
	else
		path = "html/404.html";
	return path;
}

std::string createStatusLine (std::string const & method, std::string const & uri)
{
	std::string statusLine;
	if (method == "GET" && (uri == "/" || uri == "/about" || uri == "/long"))
		statusLine = "HTTP/1.1 200 OK\r\n";
	else if (method == "GET" && (uri == "/500"))
		statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
	else if (method == "GET" && (uri == "/400"))
		statusLine = "HTTP/1.1 400 Bad Request\r\n";
	else
		statusLine = "HTTP/1.1 404 Not Found\r\n";
	return statusLine;
}

void Server::connectionType (int index)
{
	if (_clients[index].request.find ("Connection: close") != std::string::npos)
		_clients[index].keepAlive = false;
}

std::string requestURI (std::string const & request)
{
	std::istringstream stream (request);
	std::string method;
	std::string uri;
	stream >> method >> uri;
	return uri;
}

std::string requestmethod (std::string const & request)
{
	std::istringstream stream (request);
	std::string method;
	stream >> method;
	return method;
}

void Server::createResponseParts (int index)
{
	_clients[index].status = PREPARINGRESPONSE;
	connectionType (index);
	std::cout << "Creating response for client " << index + 1 << std::endl;
	std::string method = requestmethod (_clients[index].request);
	std::string uri = requestURI (_clients[index].request);
	std::string path = findPath (method, uri);
	std::string body = readFile (path);

	std::string statusLine = createStatusLine (method, uri);

	std::string contentType = "Content-Type: text/html\r\n";
	std::string connection;
	if (_clients[index].keepAlive)
		connection = "Connection: keep-alive\r\n";
	else
		connection = "Connection: close\r\n";

	std::string header;
	if (body.size () > _config.maxBodySize)
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
		if (bytes_sent < static_cast<ssize_t> (_clients[index].responseParts[0].size ()))
		{
			std::string remainPart = _clients[index].responseParts[0].substr (bytes_sent);
			_clients[index].responseParts[0] = remainPart;
			return;
		}
		else
		{
			_clients[index].responseParts.erase (_clients[index].responseParts.begin ());
			if (_clients[index].responseParts.empty ())
			{
				std::cout << "All response parts sent to client " << index + 1 << std::endl;
				if (_clients[index].keepAlive == false)
				{
					std::cout << "Client " << index + 1 << " requested to close connection" << std::endl;
					closeClientSocket (index);
				}
				else
				{
					std::cout << "Client " << index + 1 << " requested to keep connection alive. Waiting for a new rquest" << std::endl;
					_clients[index].request.clear ();
					_clients[index].status = WAITFORREQUEST;
					_clients[index].connectTime = getCurrentTime ();
				}
			}
		}
	}
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
		_clients[index].changeRequestToBadRequest();
		return 0;
	}
	if (unProcessed.size () < chunkedSize + 2)
	{
		_clients[index].changeRequestToBadRequest();
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
	if (unProcessed.find ("Transfer-Encoding: chunked") == std::string::npos)
		return (_clients[index].changeRequestToServerError ());
	if (unProcessed.find ("\r\n0\r\n\r\n") != std::string::npos)
		return (_clients[index].changeRequestToBadRequest ());
	std::string header = "";
	grabChunkedHeader (unProcessed, header, index);

	size_t chunkedSize;
	while (true)
	{
		if (unProcessed.find ("\r\n") == std::string::npos)
			return (_clients[index].changeRequestToBadRequest ());
		chunkedSize = getChunkedSize (unProcessed, index);
		if (chunkedSize == 0)
		{
			_clients[index].status = RECEIVED;
			return;
		}
		else
			grabChunkedData (unProcessed, chunkedSize, index);
	}
}

bool Server::finishedReceivingNonChunked (int index)
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
		_clients[index].changeRequestToBadRequest ();
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
		_clients[index].changeRequestToBadRequest ();
		return true;
	}
	if (receivedLength (index) > contentLength)
	{
		std::cerr << "Received more data than expected" << std::endl;
		_clients[index].changeRequestToBadRequest ();
		return true;
	}
	if (receivedLength (index) == contentLength)
		return true;
	return false;
}

bool Server::finishedReceivingChunked (int index)
{
	if (_clients[index].request.find ("\r\n0\r\n\r\n") != std::string::npos)
		return true;
	return false;
}

bool Server::finishedReceiving (int index)
{
	if (_clients[index].status == RECEIVINGUNKOWNTYPE)
		return false;
	else if (_clients[index].status == RECEIVINGCHUNKED)
		return finishedReceivingChunked (index);
	else
		return finishedReceivingNonChunked (index);
}

size_t Server::receivedLength (int index) const
{
	size_t headerLength = _clients[index].request.find ("\r\n\r\n") + 4;
	size_t receivedLength = _clients[index].request.size () - headerLength;
	return receivedLength;
}

void Server::findRequestType (int index)
{
	if (_clients[index].request.find ("\r\n\r\n") == std::string::npos)
	{
		if (_clients[index].request.size () > MAX_HEADER_SIZE)
		{
			std::cout << "Header size exceeded the limit" << std::endl;
			_clients[index].changeRequestToBadRequest ();
		}
	}
	else
	{
		if (_clients[index].request.find ("Transfer-Encoding: chunked") != std::string::npos)
			_clients[index].status = RECEIVINGCHUNKED;
		else
			_clients[index].status = RECEIVINGNONCHUNKED;
	}
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
		if (_clients[index].status == WAITFORREQUEST)
			_clients[index].status = RECEIVINGUNKOWNTYPE;
		stringBuffer = buffer;
		_clients[index].request += stringBuffer;
		if (_clients[index].status == RECEIVINGUNKOWNTYPE)
			findRequestType (index);
		if (finishedReceiving (index))
		{
			if (_clients[index].status == RECEIVINGCHUNKED)
				handleChunkedEncoding (index);
			_clients[index].status = RECEIVED;
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

void Server::handleTimeout (int index)
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

void Server::handleTimeouts ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].status > DISCONNECTED && _clients[i].status < RECEIVED && getPassedTime (i) > TIMEOUT)
			handleTimeout (i);
	}
}

void Server::prepareResponses ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].status == RECEIVED)
			createResponseParts (i);
	}
}

void Server::handleErr (struct epoll_event const & event)
{
	if (event.data.fd == _socket_fd)
	{
		std::cerr << "Error on listening socket" << std::endl;
		removeEpoll (_socket_fd);
		close (_socket_fd);
		createSocket ();
		makeSocketReusable ();
		setAddress ();
		connectToSocket ();
	}
	else
	{
		int index = getClientIndex (event);
		if (index == -1)
			return;
		std::cout << "Client " << index + 1 << " disconnected" << std::endl;
		closeClientSocket (index);
	}
}

void Server::handleClientEvents (struct epoll_event const & event)
{
	if (event.events & (EPOLLHUP | EPOLLERR))
		handleErr (event);
	else
	{
		if (getClientStatus (event) < RECEIVED && (event.events & EPOLLIN))
			receiveMessage ((ClientConnection *)event.data.ptr);
		else if (getClientStatus (event) == READYTOSEND && (event.events & EPOLLOUT))
			sendResponseParts ((ClientConnection *)event.data.ptr);
	}
}

void Server::handleListeningEvents (struct epoll_event const & event)
{
	if (event.events & (EPOLLHUP | EPOLLERR))
		handleErr (event);
	else if (event.events & EPOLLIN)
		acceptClient ();
}

void Server::handleSocketEvents ()
{
	int n_ready_fds = waitForEvents ();
	for (int i = 0; i < n_ready_fds; i++)
	{
		if (_ready[i].data.fd == _socket_fd)
			handleListeningEvents (_ready[i]);
		else
			handleClientEvents (_ready[i]);
	}
}

void Server::handleEvents ()
{
	handleSocketEvents ();
	handleTimeouts ();
	prepareResponses ();
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


time_t Server::getPassedTime (int index) const
{
	time_t current_time = getCurrentTime ();
	if (current_time == -1)
		throw SocketException ("Failed to get passed time");
	return (difftime (current_time, _clients[index].connectTime));
}

///
void Server::createSocket ()
{
	_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_socket_fd == -1)
		throw SocketException ("Failed to create socket");
}

void Server::setAddress ()
{
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo * result = nullptr;

	int error = getaddrinfo (_config.host.c_str (), nullptr, &hints, &result);
	if (error != 0)
	{
		std::string error_message = gai_strerror (error);
		throw SocketException ("Failed to get address info : " + error_message);
	}
	if (result == nullptr || result->ai_addr == nullptr)
		throw SocketException ("No address info returned");
	if (result->ai_family != AF_INET)
	{
		freeaddrinfo (result);
		throw SocketException ("Invalid address family returned");
	}
	_address = *reinterpret_cast<struct sockaddr_in *> (result->ai_addr);
	freeaddrinfo (result);
	_address.sin_port = htons (_config.port);
}

void Server::signalHandler (int signal)
{
	if (signal == SIGINT)
		signal_status = SIGINT;
}

void Server::applyCustomSignal ()
{
	if (signal (SIGINT, &signalHandler) == SIG_ERR)
		throw SocketException ("Failed to set signal handler");
}

volatile sig_atomic_t Server::signal_status = 0;

void Server::createEpoll ()
{
	_fd_epoll = epoll_create1 (0);
	if (_fd_epoll == -1)
		throw SocketException ("Failed to create epoll");
}

void Server::removeEpoll (int fd)
{
	if (epoll_ctl (_fd_epoll, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw SocketException ("Failed to remove epoll event");
}


std::string Server::readFile (std::string const & path) const
{
	std::ifstream file (path.c_str ());
	if (!file.is_open ())
		throw SocketException ("Failed to open file");
	std::stringstream read;
	read << file.rdbuf ();
	file.close ();
	return read.str ();
}

void Server::makeSocketReusable ()
{
	int reusable = 1;
	if (setsockopt (_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof (reusable)) == -1)
		throw SocketException ("Failed to make socket reusable");
}

time_t Server::getCurrentTime () const
{
	time_t current_time = time (nullptr);
	if (current_time == -1)
		throw SocketException ("Failed to get current time");
	return current_time;
}
