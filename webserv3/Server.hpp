#ifndef SERVER_HPP
#define SERVER_HPP

#include <cerrno>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "SocketException.hpp"
#include "Configration.hpp"
#include "ClientConnection.hpp"

class Server
{

      private:

	/// Constants
	static constexpr int MAX_CONNECTIONS = 5;
	static constexpr int BACKLOG = (2 * MAX_CONNECTIONS);
	const size_t MAX_HEADER_SIZE = 32768;
	static constexpr int TIMEOUT = 10;

	/// Private Attributes
	int _socket_fd;
	int _fd_epoll;
	struct sockaddr_in _address;
	Configration _config;
	int _num_clients;
	ClientConnection _clients[MAX_CONNECTIONS];
	struct epoll_event _events[MAX_CONNECTIONS + 1];
	struct epoll_event _ready[MAX_CONNECTIONS + 1];

	/// Private Methods
	std::string requestURI (std::string const & message) const;
	std::string requestmethod (std::string const & message) const;
	time_t getPassedTime (int index) const;
	int getClientIndex (struct epoll_event const & event) const;
	void handleChunkedEncoding (int index);
	void changeRequestToBadRequest (int index);
	void changeRequestToServerError (int index);
	void grabChunkedHeader (std::string & unProcessed, std::string & header, int index);
	size_t getChunkedSize (std::string & unProcessed, int index);
	void grabChunkedData (std::string & unProcessed, size_t chunkedSize, int index);
	void connectionType (int index);
	bool finishedReceiving (int index);
	bool finishedReceivingChunked (int index);
	bool finishedReceivingNonChunked (int index);
	size_t receivedLength (int index) const;
	void handleTimeouts ();
	void prepareResponses ();
	void handleSocketEvents ();
	void handleErr (struct epoll_event const & event);
	void handleClientEvents (struct epoll_event const & event);
	void handleListeningEvents (struct epoll_event const & event);
	void findRequestType (int index);
	void setAddress ();
	void createSocket ();
	static void signalHandler (int signal);
	void createEpoll ();
	void removeEpoll (int fd);
	void applyCustomSignal ();
	std::string readFile (std::string const & path) const;
	void makeSocketReusable ();
	time_t getCurrentTime () const;
	void acceptClient ();
	std::string getRequest (int index) const;
	void closeClientSocket (int index);
	void closeClientSockets ();
	int getNumClients () const;
	void addEpoll (int fd, int index);
	void sendResponseParts (ClientConnection * client);
	void receiveMessage (ClientConnection * client);
	int waitForEvents ();
	std::string findPath (std::string const & method, std::string const & uri) const;
	void createResponseParts (int index);
	std::string createStatusLine (std::string const & method, std::string const & uri) const;
	int getClientStatus (struct epoll_event const & event) const;
	void handleTimeout (int index);

      public:
	
	/// Public Methods
	Server (int port, std::string const & host, size_t maxBodySize, std::map<std::string, std::string> const & routes);
	void connectToSocket ();
	void handleEvents ();
	void closeSocket ();

	/// Static Attributes
	static volatile sig_atomic_t signal_status;
};

#endif