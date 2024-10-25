#include "Socket.hpp"

Socket::Socket (int port, std::string const & host, std::map<std::string, std::string> routes)
    : _socket_fd(-1), _fd_epoll(-1), _config(port, host, routes)
{
	applyCustomSignal ();
	createSocket ();
	makeSocketReusable ();
	setReceiveTimeout ();
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

void Socket::applyCustomSignal ()
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
	: index(-1), fd (-1), connected (false), status(DISCONNECTED){};

Socket::SocketException::SocketException(std::string const & message)
	: std::runtime_error (message + " : " + strerror(errno)){};

Socket::Configration::Configration(int port, std::string const & host, std::map<std::string, std::string> routes): port(port), host(host), routes(routes){};


std::string Socket::readFile(std::string const & path) const
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw SocketException ("Failed to open file");
	std::stringstream read;
	read << file.rdbuf();
	file.close();
	return read.str();
}

void Socket::makeSocketReusable()
{
    int reusable = 1;
    if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof(reusable)) == -1)
        throw SocketException ("Failed to make socket reusable");
}

void Socket::setReceiveTimeout()
{
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	if (setsockopt(_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
		throw SocketException ("Failed to set receive timeout");
}


