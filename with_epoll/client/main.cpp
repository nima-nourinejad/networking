#include "Client.hpp"

int main (int argc, char ** argv)
{
	(void)argc;
	Client client (9001, argv[1]);
	client.customSignal ();
	client.makeSocketNonBlocking ();
	while (client.signal_status != SIGINT)
	{
		client.connectToSocket ();
		client.handleEvents ();
		
	}
	client.closeSocket ();
	return 0;
}
