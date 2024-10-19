#include "Server.hpp"
#include <sys/epoll.h>

int main()
{
    Server server(9001);
	server.makeSocketNonBlocking();
	server.connectToSocket();

	
	std::string message;
	while (true)
	{
		server.acceptClient();
		if (server.getNumClients() > 0)
		{
			server.receiveMessage();
			std::getline(std::cin, message);
			if (std::cin.eof() || message == "exit")
				break;
			server.sendMessage(message);
		}
	}

	server.closeSocket();
    return 0;
}
