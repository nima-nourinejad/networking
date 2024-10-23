#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

class Socket
{
    protected:

		class ClientConnection
		{
			public:
				int index;
				int fd;
				bool connected;
				bool sent;
				bool received;
				std::string message;
				ClientConnection () : index(0), fd (-1), connected (false), sent (false), received (false){};
		};
		class SocketException : public std::runtime_error
		{
			public:
				SocketException (std::string const & message)
					: std::runtime_error (message + " : " + strerror(errno)){};
		};
		int _socket_fd;
		struct sockaddr_in _address;
		const int _port;
		std::string const _name;
		int _fd_epoll;

    public:
		Socket (int port, std::string const & name);
		void setAddress ();
		void createSocket ();

		virtual void connectToSocket () = 0;
		virtual void closeSocket () = 0;
		int getSocketFD () const;
		std::string getName() const;
		void customSignal();
		static void signalHandler(int signal);
		static volatile sig_atomic_t signal_status;
		void createEpoll();
		void removeEpoll(int fd);
};

#endif