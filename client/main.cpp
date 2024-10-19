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
		getline (std::cin, message);
		if (std::cin.eof ())
			break;
		if (client.isConnected ())
		{
			client.sendMessage (message);
			client.receiveMessage ();
			std::cout << client.getName () << ": I received this message from server:" << std::endl;
			std::cout << client.getMessage () << std::endl;
		}
	}
	client.closeSocket ();
	return 0;
}
