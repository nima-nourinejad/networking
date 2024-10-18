#include "Client.hpp"

int main ()
{
	Client client1 (9001);
	client1.connectToSocket ();

	std::string message;
	while (true)
	{
		getline (std::cin, message);
		if (std::cin.eof ())
			break;
		client1.sendMessage (message);
		client1.receiveMessage ();
		std::cout << "Server: " << client1.getMessage () << std::endl;
	}

	client1.closeSocket ();
	return 0;
}
