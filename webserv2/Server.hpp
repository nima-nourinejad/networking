#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include "Client.hpp"


class Server : public Socket, public Client
{

    private:
		static constexpr int MAX_CONNECTIONS = 5;
		static constexpr int BACKLOG = (2 * MAX_CONNECTIONS);
		static constexpr int TIMEOUT = 10;
		int _num_clients;
		
		
		// std::string requestURI(std::string const & message) const;
		// std::string requestmethod(std::string const & message) const;
		// time_t getPassedTime(int index) const;
		int getClientIndex(struct epoll_event const & event) const;
		// void handleChunkedEncoding(int index);
		// void changeRequestToBadRequest(int index);
		// void changeRequestToServerError(int index);
		// void grabChunkedHeader(std::string & unProcessed, std::string & header, int index);
		// size_t getChunkedSize(std::string & unProcessed, int index);
		// void grabChunkedData(std::string & unProcessed, size_t chunkedSize, int index);
		// void connectionType(int index);
		// bool finishedReceiving(int index);
		// bool finishedReceivingChunked(int index);
		// bool finishedReceivingNonChunked(int index);
		// size_t receivedLength(int index) const;
		// void handleTimeouts();
		void prepareResponses();
		void handleSocketEvents();
		void handleErr(struct epoll_event const & event);
		void handleClientEvents(struct epoll_event const & event);
		void handleListeningEvents(struct epoll_event const & event);
		// void findRequestType(int index);
		void acceptClient ();
		Client _clients[MAX_CONNECTIONS];

		

    public:
		Server (int port, std::string const & host, size_t maxBodySize, std::map<std::string, std::string> const & routes);
		void connectToSocket () override;
		
		// std::string getRequest (int index) const;
		void closeSocket () override;
		void closeClientSocket(int index);
		void closeClientSockets ();
		// int getNumClients () const;
		void handleEvents();
		void addEpoll(int fd, int index);
		// void sendResponseParts(ClientConnection * client);
		// void receiveMessage(ClientConnection * client);
		int waitForEvents();
		// std::string findPath(std::string const & method, std::string const & uri) const;
		// void createResponseParts(int index);
		// std::string createStatusLine(std::string const & method, std::string const & uri) const;
		int getClientStatus(struct epoll_event const & event) const;
		
		struct epoll_event _events[MAX_CONNECTIONS + 1];
		struct epoll_event _ready[MAX_CONNECTIONS + 1];
		// void handleTimeout(int index);
		
		
};

#endif