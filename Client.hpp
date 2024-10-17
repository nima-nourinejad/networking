#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include <iostream>

class Client : public Socket {

	private:
		std::string _message;
	public:
		Client(int port);
		void connectToSocket() override;
		void receiveMessage();
		void showReceivedMessage();
};

#endif