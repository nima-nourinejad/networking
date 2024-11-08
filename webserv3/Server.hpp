#ifndef SERVER_HPP
#define SERVER_HPP

#include <cerrno>
#include <fstream>
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

	/// ClientConnection Methods
	time_t getPassedTime (int index) const;
	void prepareResponses ();
	void closeClientSocket (int index);
	void closeClientSockets ();
	void sendResponseParts (ClientConnection * client);
	void receiveMessage (ClientConnection * client);
	void createResponseParts (int index);
	void handleTimeout (int index);

	/// Event Handling Methods
	int getClientIndex (struct epoll_event const & event) const;
	void handleTimeouts ();
	void handleSocketEvents ();
	void handleErr (struct epoll_event const & event);
	void handleClientEvents (struct epoll_event const & event);
	void handleListeningEvents (struct epoll_event const & event);
	void createEpoll ();
	void removeEpoll (int fd);
	void addEpoll (int fd, int index);
	int waitForEvents ();
	int getClientStatus (struct epoll_event const & event) const;
	

	/// Utility Methods
	static void signalHandler (int signal);
	void applyCustomSignal ();
	std::string readFile (std::string const & path) const;
	time_t getCurrentTime () const;

	/// Listening Socket Methods
	void setAddress ();
	void createSocket ();
	void makeSocketReusable ();
	void acceptClient ();
	int getNumClients () const;

      public:
	/// Listening Socket Methods
	Server (int port, std::string const & host, size_t maxBodySize);
	void connectToSocket ();
	void handleEvents ();
	void closeSocket ();

	/// Static Attributes
	static volatile sig_atomic_t signal_status;
};

#endif