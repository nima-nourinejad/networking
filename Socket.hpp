#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <fcntl.h>

class Socket
{
	protected:
		int _socket_fd;
		int _client_fd[2];
		int _num_clients;

		struct sockaddr_in _address;
		const int _port;

		std::string _message[2];
	public:
		Socket(int port);
		void setAddress();
		void createSocket();

		void sendMessage(std::string message, int index);
		void receiveMessage(int index);
		void showReceivedMessage(int index);
		virtual void connectToSocket() = 0;
		void closeSocket();
		void closeClientSocket();
		int getLastClientAdded() const;
		int getClient(int index) const;
		int getNumClients() const;

		std::string getMessage(int index) const;
};

#endif