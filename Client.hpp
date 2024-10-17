#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"

class Client : public Socket {

	public:
		Client(int port);
		void connectToSocket() override;
};

#endif