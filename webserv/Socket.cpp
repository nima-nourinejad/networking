#include "Socket.hpp"

Socket::Socket (int port, std::string const & name)
    : _port (port), _name (name)
{
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
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons (_port);
}

int Socket::getSocketFD () const
{
	return _socket_fd;
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

std::string Socket::getName() const
{
	return _name;
};

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
