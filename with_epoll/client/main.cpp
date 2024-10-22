#include "Client.hpp"

int main (int argc, char ** argv)
{
	(void)argc;
	Client client (9001, argv[1]);
	client.customSignal ();
	client.makeSocketNonBlocking ();
	std::string message;
	while (client.signal_status != SIGINT)
	{
		client.connectToSocket ();
		if(client.isConnected ())
		{
			if (!client.isSent())
			{
				std::cout << "Enter a message to send to server: ";
				std::getline (std::cin, message);
				if (std::cin.eof ())
					break;
				client.sendMessage (message);
			}
			if (client.isSent ())
				client.receiveMessage ();
		}
	}
	client.closeSocket ();
	return 0;
}
