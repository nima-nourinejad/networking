#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <cerrno>
#include <sys/epoll.h>
#include <map>
#include <netdb.h>


class Socket
{
    protected:

		class Configration
		{
			public:
				int port;
				std::string host;
				std::string serverName;
				std::string errorPage;
				size_t clientMaxBodySize;
				std::map<std::string, std::string> routes;
				Configration(int port, std::string const & host);
		};

		class ClientConnection
		{
			public:
				int index;
				int fd;
				bool connected;
				std::string message;
				ClientConnection ();
		};

		class SocketException : public std::runtime_error
		{
			public:
				SocketException (std::string const & message);
		};

		Configration _config;
		int _socket_fd;
		int _fd_epoll;
		struct sockaddr_in _address;
		virtual void connectToSocket () = 0;
		void setAddress ();
		void createSocket ();
		static void signalHandler(int signal);
		void createEpoll();
		void removeEpoll(int fd);
		void customSignal();

    public:
		Socket (int port, std::string const & host);
		virtual void closeSocket () = 0;
		static volatile sig_atomic_t signal_status;

};

#endif