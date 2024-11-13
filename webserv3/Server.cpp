#include "Server.hpp"

Server::Server (int port, std::string const & host, size_t maxBodySize)
    : _socket_fd (-1), _fd_epoll (-1), _config (port, host, maxBodySize), _num_clients (0)
{
	applyCustomSignal ();
	createEpoll ();
	startListeningSocket ();
	setClientsMaxBodySize (maxBodySize);
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

bool Server::serverFull () const
{
	if (_num_clients >= MAX_CONNECTIONS)
	{
		std::cout << "Max clients reached" << std::endl;
		return true;
	}
	return false;
}

int Server::findAvailableSlot () const
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].status == DISCONNECTED && _clients[i].fd == -1 && _clients[i].index == -1)
			return i;
	}
	return -1;
}

void Server::occupyClientSlot (int availableSlot, int fd)
{
	std::cout << "Accepted client " << availableSlot + 1 << ". Waiting for the rquest" << std::endl;
	_clients[availableSlot].fd = fd;
	_clients[availableSlot].index = availableSlot;
	_clients[availableSlot].status = WAITFORREQUEST;
	_clients[availableSlot].setCurrentTime ();
}

void Server::handlePendingConnections ()
{
	while (true)
	{
		int availableSlot = findAvailableSlot ();
		if (availableSlot == -1)
			throw SocketException ("Failed to find available slot for client");
		int fd = accept (_socket_fd, nullptr, nullptr);
		if (fd == -1)
		{
			if (errno != EAGAIN)
				throw SocketException ("Failed to accept client");
			else
			{
				std::cout << "No pending connections anymore" << std::endl;
				break;
			}
		}
		else
		{
			addEpoll (fd, availableSlot);
			++_num_clients;
			occupyClientSlot (availableSlot, fd);
		}
	}
}

void Server::acceptClient ()
{
	std::cout << "There are pending connections" << std::endl;
	if (serverFull ())
	{
		ClientConnection::sendServiceUnavailable (_socket_fd, _config.maxBodySize);
		return;
	}
	try
	{
		handlePendingConnections ();
	}
	catch (SocketException const & e)
	{
		e.log ();
		if (e.type == ADD_EPOLL)
		{
			if (e.open_fd != -1)
				ClientConnection::sendServerError (e.open_fd, _config.maxBodySize);
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
					_clients[index].setCurrentTime ();
				}
			}
		}
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
			_clients[index].findRequestType ();
		if (_clients[index].finishedReceiving ())
		{
			if (_clients[index].status == RECEIVINGCHUNKED)
				_clients[index].handleChunkedEncoding ();
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
		_clients[index].createResponseParts ();
	}
	else
		closeClientSocket (index);
}

void Server::handleTimeouts ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].status > DISCONNECTED && _clients[i].status < RECEIVED && _clients[i].getPassedTime () > TIMEOUT)
			handleTimeout (i);
	}
}

void Server::prepareResponses ()
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (_clients[i].status == RECEIVED)
			_clients[i].createResponseParts ();
	}
}

void Server::handleErr (struct epoll_event const & event)
{
	if (event.data.fd == _socket_fd)
	{
		std::cerr << "Error on listening socket" << std::endl;
		removeEpoll (_socket_fd);
		close (_socket_fd);
		startListeningSocket ();
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
		throw SocketException ("Failed to add to epoll", fd);
}

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

void Server::makeSocketReusable ()
{
	int reusable = 1;
	if (setsockopt (_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof (reusable)) == -1)
		throw SocketException ("Failed to make socket reusable");
}

void Server::setClientsMaxBodySize (size_t maxBodySize)
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
		_clients[i].maxBodySize = maxBodySize;
}

void Server::startListeningSocket()
{

	_retry = 0;
	bool success = false;

	while (signal_status != SIGINT && !success && _retry < MAX_RETRY)
	{
		try
		{
			createSocket ();
			makeSocketReusable ();
			setAddress ();
			connectToSocket ();
			success = true;
		}
		catch (SocketException const & e)
		{
			e.log ();
			if (_socket_fd != -1)
				close (_socket_fd);
			++_retry;
		}
	}
}