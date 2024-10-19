#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include <string>

class Server : public Socket
{

      private:
	static constexpr int MAX_CONNECTIONS = 2;
	int _client_fd[MAX_CONNECTIONS];
	std::string _message[MAX_CONNECTIONS];
	int _num_clients;

      public:
	Server (int port);
	void connectToSocket () override;
	void acceptClient ();
	void sendMessage (std::string message);
	void receiveMessage ();
	std::string getMessage (int index) const;
	void closeSocket () override;
	void closeClientSockets ();
	int getNumClients () const;
	int getClientFD (int index) const;
	int getMaxConnections () const;
};

#endif