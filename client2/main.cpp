#include "Client.hpp"

int main ()
{
	Client client2 (9001);
	client2.makeSocketNonBlocking ();

	std::string message;
	while (true)
	{
		client2.connectToSocket ();
		getline (std::cin, message);
		if (std::cin.eof () || message == "exit")
			break;
		if (client2.isConnected ())
		{

			client2.sendMessage (message);
			client2.receiveMessage ();
			std::cout << "Client2: I received this message from server:" << std::endl;
			std::cout << client2.getMessage () << std::endl;
		}
	}

	client2.closeSocket ();
	return 0;
}
