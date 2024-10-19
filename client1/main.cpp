#include "Client.hpp"

int main ()
{
	Client client1 (9001);
	client1.makeSocketNonBlocking ();

	std::string message;
	while (true)
	{
		client1.connectToSocket ();
		getline (std::cin, message);
		if (std::cin.eof () || message == "exit")
			break;
		if (client1.isConnected ())
		{

			client1.sendMessage (message);
			client1.receiveMessage ();
			std::cout << "Client1: I received this message from server:" << std::endl;
			std::cout << client1.getMessage () << std::endl;
		}
	}

	client1.closeSocket ();
	return 0;
}
