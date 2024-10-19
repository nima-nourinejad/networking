#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"

class Client : public Socket
{
	private:
		std::string _message;
		bool _connected;
      public:
	Client (int port);
	void connectToSocket () override;
	void closeSocket () override;
	void sendMessage (std::string message);
	void receiveMessage ();
	std::string getMessage () const;
	bool isConnected () const;
};

#endif