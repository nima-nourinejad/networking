#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdexcept>

// #define IPv4 = AF_INET;
// #define TCP = SOCK_STREAM;
// #define LISTEN_TO_ALL = INADDR_ANY;

class Socket
{
	protected:
		
		static constexpr int NORMAL = 0;
		int _socket_fd;
		struct sockaddr_in _address;
		const int _port;
	public:
		Socket(int port);
		void setAddress();
		void createSocket();
		virtual void connectToSocket() = 0;
		// void closeSocket();
};

#endif