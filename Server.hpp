#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include <string>

class Server : public Socket {

	private:
		static constexpr int MAX_CONNECTIONS = 2;
	public:
		Server(int port);
		void connectToSocket() override;
		void getNewClient();
		
};

#endif