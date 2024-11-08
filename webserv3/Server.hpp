#ifndef SERVER_HPP
#define SERVER_HPP

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
#include <vector>
#include <ctime>


class Server
{

    private:
		static constexpr int MAX_CONNECTIONS = 5;
		static constexpr int BACKLOG = (2 * MAX_CONNECTIONS);
		const size_t MAX_HEADER_SIZE = 32768;
		static constexpr int TIMEOUT = 10;
		
		
		
		std::string requestURI(std::string const & message) const;
		std::string requestmethod(std::string const & message) const;
		time_t getPassedTime(int index) const;
		int getClientIndex(struct epoll_event const & event) const;
		void handleChunkedEncoding(int index);
		void changeRequestToBadRequest(int index);
		void changeRequestToServerError(int index);
		void grabChunkedHeader(std::string & unProcessed, std::string & header, int index);
		size_t getChunkedSize(std::string & unProcessed, int index);
		void grabChunkedData(std::string & unProcessed, size_t chunkedSize, int index);
		void connectionType(int index);
		bool finishedReceiving(int index);
		bool finishedReceivingChunked(int index);
		bool finishedReceivingNonChunked(int index);
		size_t receivedLength(int index) const;
		void handleTimeouts();
		void prepareResponses();
		void handleSocketEvents();
		void handleErr(struct epoll_event const & event);
		void handleClientEvents(struct epoll_event const & event);
		void handleListeningEvents(struct epoll_event const & event);
		void findRequestType(int index);

		

    public:

	class SocketException : public std::runtime_error
		{
			public:
				SocketException (std::string const & message);
		};

		int _socket_fd;
		int _fd_epoll;
		struct sockaddr_in _address;
		void setAddress ();
		void createSocket ();
		static void signalHandler(int signal);
		void createEpoll();
		void removeEpoll(int fd);
		
		void applyCustomSignal();
		std::string readFile(std::string const & path) const;
		void makeSocketReusable();
		time_t getCurrentTime() const;
		

    

		enum
		{
			DISCONNECTED,
			WAITFORREQUEST,
			RECEIVINGUNKOWNTYPE,
			RECEIVINGNONCHUNKED,
			RECEIVINGCHUNKED,
			RECEIVED,
			PREPARINGRESPONSE,
			READYTOSEND
		};
	
		class Configration
		{
			public:
				int port;
				std::string host;
				size_t maxBodySize;
				std::map <std::string, std::string> routes;
				Configration(int port, std::string const & host, size_t maxBodySize, std::map<std::string, std::string> const & routes);
		};
		class ClientConnection
			{
				public:
					int index;
					int fd;
					int status;
					bool keepAlive;
					time_t connectTime;
					std::string request;
					std::vector<std::string> responseParts;
					ClientConnection ();
			};
		Configration _config;
		int _num_clients;
		static volatile sig_atomic_t signal_status;



	////
		Server (int port, std::string const & host, size_t maxBodySize, std::map<std::string, std::string> const & routes);
		void connectToSocket ();
		void acceptClient ();
		std::string getRequest (int index) const;
		void closeSocket ();
		void closeClientSocket(int index);
		void closeClientSockets ();
		int getNumClients () const;
		void handleEvents();
		void addEpoll(int fd, int index);
		void sendResponseParts(ClientConnection * client);
		void receiveMessage(ClientConnection * client);
		int waitForEvents();
		std::string findPath(std::string const & method, std::string const & uri) const;
		void createResponseParts(int index);
		std::string createStatusLine(std::string const & method, std::string const & uri) const;
		int getClientStatus(struct epoll_event const & event) const;
		ClientConnection _clients[MAX_CONNECTIONS];
		struct epoll_event _events[MAX_CONNECTIONS + 1];
		struct epoll_event _ready[MAX_CONNECTIONS + 1];
		void handleTimeout(int index);



		/////
		
	
		
		
};

#endif