#include "Client.hpp"

int main ()
{
	Client client2 (9001);
	client2.connectToSocket ();

	std::string message;
	while (true)
	{
		getline (std::cin, message);
		if (std::cin.eof ())
			break;
		client2.sendMessage (message);
		client2.receiveMessage ();
		std::cout << "Server: " << client2.getMessage () << std::endl;
	}

	client2.closeSocket ();
	return 0;
}
