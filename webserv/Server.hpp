#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"


class Server : public Socket
{

    private:
		static constexpr int MAX_CONNECTIONS = 5;
		static constexpr int BACKLOG = (2 * MAX_CONNECTIONS);
		int _num_clients;
		
		
		std::string requestURI(std::string const & message) const;
		std::string requestmethod(std::string const & message) const;
		time_t getPassedTime(int index) const;
		int getClientIndex(struct epoll_event const & event) const;
		// void modifyEpoll(int index, uint32_t status);
		

    public:
		Server (int port, std::string const & host, std::map<std::string, std::string> routes);
		void connectToSocket () override;
		void acceptClient ();
		std::string getRequest (int index) const;
		void closeSocket () override;
		void closeClientSocket(int index);
		void closeClientSockets ();
		int getNumClients () const;
		void handleEvents();
		// void addEpoll(int fd, struct epoll_event * event, int index);
		void addEpoll(int fd, int index);
		void sendMessage(ClientConnection * client);
		void receiveMessage(ClientConnection * client);
		bool receiveMessage(ClientConnection & client);
		int waitForEvents();
		std::string finfPath(std::string const & method, std::string const & uri) const;
		void createResponse(int index);
		int getClientStatus(struct epoll_event const & event) const;
		void showActiveClients();
		ClientConnection _clients[MAX_CONNECTIONS];
		struct epoll_event _events[MAX_CONNECTIONS + 1];
		struct epoll_event _ready[MAX_CONNECTIONS + 1];
		
		
};

#endif