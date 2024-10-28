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
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

class Socket
{
    protected:
		class ClientConnection
			{
				public:
					int index;
					int fd;
					bool connected;
					int status;
					time_t lastActivity;
					std::string request;
					std::string response;
					ClientConnection ();
			};
		class SocketException : public std::runtime_error
		{
			public:
				SocketException (std::string const & message);
		};

		int _socket_fd;
		int _fd_epoll;
		struct sockaddr_in _address;
		virtual void connectToSocket () = 0;
		void setAddress ();
		void createSocket ();
		static void signalHandler(int signal);
		void createEpoll();
		void removeEpoll(int fd);
		
		void applyCustomSignal();
		std::string readFile(std::string const & path) const;
		void makeSocketReusable();
		time_t getCurrentTime() const;
		

    public:

		enum
		{
			DISCONNECTED,
			CONNECTED,
			RECEIVED,
			PROCESSING,
			READYTOSEND,
			SENT
		};
	
		class Configration
		{
			public:
				int port;
				std::string host;
				std::string errorPage;
				size_t maxBodySize;
				std::map <std::string, std::string> routes;
				Configration(int port, std::string const & host, std::string const & errorPage, size_t maxBodySize, std::map<std::string, std::string> const & routes);
		};
		Configration _config;
		Socket (int port, std::string const & host, std::string const & errorPage, size_t maxBodySize, std::map<std::string, std::string> const & routes);
		virtual void closeSocket () = 0;
		static volatile sig_atomic_t signal_status;
	

};

#endif