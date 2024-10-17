#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include <string>

class Server : public Socket {

	private:
		static constexpr int MAX_CONNECTIONS = 1;
		int _client_fd;
	public:
		Server(int port);
		void connectToSocket() override;
		void getNewClient();
		void sendMessage(std::string message);
};

#endif