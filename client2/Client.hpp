#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"

class Client : public Socket
{
	private:
		std::string _message;
      public:
	Client (int port);
	void connectToSocket () override;
	void closeSocket () override;
	void sendMessage (std::string message);
	void receiveMessage ();
	std::string getMessage () const;
};

#endif