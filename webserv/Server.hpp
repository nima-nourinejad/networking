#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include <string>

class Server : public Socket
{

    private:
		static constexpr int MAX_CONNECTIONS = 2;
		static constexpr int BACKLOG = 2 * MAX_CONNECTIONS;
		int _num_clients;
		ClientConnection _clients[MAX_CONNECTIONS];
		struct epoll_event _events[MAX_CONNECTIONS];

    public:
		Server (int port);
		void connectToSocket () override;
		void acceptClient ();
		std::string getMessage (int index) const;
		void closeSocket () override;
		void closeClientSockets ();
		int getNumClients () const;
		int getClientFD (int index) const;
		int getMaxConnections () const;
		void handleEvents();
		void addEpollEvent(int fd, struct epoll_event * event, int index);
		void sendMessage(ClientConnection * client);
		void receiveMessage(ClientConnection * client);
		struct epoll_event * getEvents();
		int howManyEventsShouldbeHandled();
};

#endif