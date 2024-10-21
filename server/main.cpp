#include "Server.hpp"
#include <sys/epoll.h>

int main ()
{
	Server server (9001);
	server.customSignal ();
	server.makeSocketNonBlocking ();
	server.connectToSocket ();

	std::string message;
	while (server.signal_status != SIGINT)
	{
		server.acceptClient ();
		if (server.getNumClients () > 0)
		{
			if (server.numRecievedMessages () < server.getNumClients ())
			{
				std::cout << "Waiting for messages" << std::endl;
				server.receiveMessage ();
			}
			if (server.numSentMessages () < server.getNumClients ())
			{
				std::cout << "Enter message to send to the clients: ";
				std::getline (std::cin, message);
				if (std::cin.eof ())
					break;
				server.sendMessage (message);
			}
			
		}
	}

	server.closeSocket ();
	return 0;
}
