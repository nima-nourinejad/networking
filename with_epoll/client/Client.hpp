#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include <memory>

class Client : public Socket
{
	private:
		std::string _message;
		ClientConnection _client;
    public:
		Client (int port, std::string const & name);
		void connectToSocket () override;
		void closeSocket () override;
		void sendMessage (std::string message);
		void receiveMessage ();
		std::string getMessage () const;
		bool isConnected () const;
		bool isReceived () const;
		bool isSent () const;
	
};

#endif