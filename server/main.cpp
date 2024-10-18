#include "Server.hpp"
#include <sys/epoll.h>

int main()
{
    Server server(9001);
	server.connectToSocket();

	while (server.getNumClients() < server.getMaxConnections())
		server.acceptClient();
	
	for (int i = 0; i < server.getNumClients(); i++)
	{
		server.receiveMessage(i);
		std::cout << "Client " << i << " says: " << server.getMessage(i) << std::endl;
		server.sendMessage("Hello from server", i);
	}

	server.closeSocket();
    return 0;
}
