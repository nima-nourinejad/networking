#include "Socket.hpp"

Socket::Socket (int port, std::string const & host)
    : _config(port, host), _socket_fd(-1), _fd_epoll(-1)
{
	customSignal ();
	createSocket ();
	setAddress ();
	createEpoll();
}

void Socket::createSocket ()
{
	_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_socket_fd == -1)
		throw SocketException ("Failed to create socket");
}

void Socket::setAddress ()
{	
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo * result = nullptr;

	int error = getaddrinfo(_config.host.c_str(), nullptr, &hints, &result);
	if (error != 0)
	{
		std::string error_message = gai_strerror(error);
		throw SocketException ("Failed to get address info : " + error_message);
	}
	if (result == nullptr || result->ai_addr == nullptr)
    	throw SocketException("No address info returned");
	if (result->ai_family != AF_INET)
	{
		freeaddrinfo(result);
		throw SocketException("Invalid address family returned");
	}
	_address = *reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
	freeaddrinfo(result);
	_address.sin_port = htons (_config.port);
}


void Socket::signalHandler (int signal)
{
	if (signal == SIGINT)
		signal_status = SIGINT;
}

void Socket::customSignal ()
{
	if (signal (SIGINT, &signalHandler) == SIG_ERR)
		throw SocketException ("Failed to set signal handler");
}

volatile sig_atomic_t Socket::signal_status = 0;

void Socket::createEpoll()
{
	_fd_epoll = epoll_create1(0);
	if (_fd_epoll == -1)
		throw SocketException ("Failed to create epoll");
}

void Socket::removeEpoll(int fd)
{
	if (epoll_ctl(_fd_epoll, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw SocketException ("Failed to remove epoll event");
}

Socket::ClientConnection::ClientConnection()
	: index(-1), fd (-1), connected (false){};

Socket::SocketException::SocketException(std::string const & message)
	: std::runtime_error (message + " : " + strerror(errno)){};

Socket::Configration::Configration(int port, std::string const & host): port(port), host(host){};
