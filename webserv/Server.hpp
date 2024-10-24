#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"


class Server : public Socket
{

    private:
		static constexpr int MAX_CONNECTIONS = 2;
		static constexpr int BACKLOG = 2 * MAX_CONNECTIONS;
		int _num_clients;
		ClientConnection _clients[MAX_CONNECTIONS];
		struct epoll_event _events[MAX_CONNECTIONS];
		std::string requestURI(std::string const & message) const;
		std::string requestmethod(std::string const & message) const;

    public:
		Server (int port, std::string const & host, std::map<std::string, std::string> routes);
		void connectToSocket () override;
		void acceptClient ();
		std::string getMessage (int index) const;
		void closeSocket () override;
		void closeClientSocket(int index);
		void closeClientSockets ();
		int getNumClients () const;
		int getClientFD (int index) const;
		int getMaxConnections () const;
		void handleEvents();
		void addEpoll(int fd, struct epoll_event * event, int index);
		void sendMessage(ClientConnection * client);
		void receiveMessage(ClientConnection * client);
		struct epoll_event * getEvents();
		int waitForEvents();
		std::string finfPath(std::string const & method, std::string const & uri) const;
		std::string createResponse(std::string const & method, std::string const & uri, std::string const & bdoy) const;
		
};

#endif