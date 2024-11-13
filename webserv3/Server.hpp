#ifndef SERVER_HPP
#define SERVER_HPP

#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>

#include "Configration.hpp"
#include "ClientConnection.hpp"

class Server
{

      private:

	/// Constants
	static constexpr int MAX_CONNECTIONS = 5;
	static constexpr int BACKLOG = (2 * MAX_CONNECTIONS);
	static constexpr int TIMEOUT = 10;
	static constexpr int MAX_RETRY = 5;

	/// Private Attributes
	int _socket_fd;
	int _fd_epoll;
	struct sockaddr_in _address;
	Configration _config;
	int _num_clients;
	ClientConnection _clients[MAX_CONNECTIONS];
	struct epoll_event _events[MAX_CONNECTIONS + 1];
	struct epoll_event _ready[MAX_CONNECTIONS + 1];
	int _retry;

	/// ClientConnection Methods
	void occupyClientSlot (int availbleSlot, int fd);
	void closeClientSockets ();
	void handleTimeout (int index);
	void closeClientSocket (int index);
	void setClientsMaxBodySize (size_t maxBodySize);
	void receiveMessage (ClientConnection * client);
	void sendResponseParts (ClientConnection * client);

	/// Event Handling Methods
	int waitForEvents ();
	int findAvailableSlot () const;
	int getClientStatus (struct epoll_event const & event) const;
	int getClientIndex (struct epoll_event const & event) const;
	bool serverFull () const;
	void createEpoll ();
	void handleTimeouts ();
	void prepareResponses ();
	void removeEpoll (int fd);
	void handleSocketEvents ();
	void handlePendingConnections ();
	void addEpoll (int fd, int index);
	void handleErr (struct epoll_event const & event);
	void handleClientEvents (struct epoll_event const & event);
	void handleListeningEvents (struct epoll_event const & event);
	
	/// Signal Methods
	void applyCustomSignal ();
	static void signalHandler (int signal);

	/// Listening Socket Methods
	void setAddress ();
	void createSocket ();
	void makeSocketReusable ();
	void connectToSocket ();
	void acceptClient ();
	void startListeningSocket ();
	
      public:
	/// Main Methods
	Server (int port, std::string const & host, size_t maxBodySize);
	void handleEvents ();
	void closeSocket ();

	/// Static Attributes
	static volatile sig_atomic_t signal_status;
};

#endif