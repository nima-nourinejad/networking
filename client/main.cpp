#include "Client.hpp"

int main (int argc, char ** argv)
{
	(void)argc;
	int i = 1;
	Client client (9001, argv[1]);
	client.customSignal ();
	client.makeSocketNonBlocking ();
	std::string message;
	while (client.signal_status != SIGINT)
	{
		client.connectToSocket ();
		client.sendMessage ("message_" + std::to_string (i));
		client.receiveMessage ();
		std::cout << client.getName () << ": I received this message from server:" << std::endl;
		std::cout << client.getMessage () << std::endl;
		i++;
	}
	client.closeSocket ();
	return 0;
}
