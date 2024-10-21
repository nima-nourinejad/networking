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
		int numRecievedMessages() const;
		int numSentMessages() const;
};

#endif