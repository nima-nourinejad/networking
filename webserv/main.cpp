#include "Server.hpp"
#include <sys/epoll.h>

int main ()
{
	Server server (9001);
	server.customSignal ();
	server.connectToSocket ();

	try
	{
		while (server.signal_status != SIGINT)
		{
			server.acceptClient ();
			server.handleEvents ();
		}
	}
	catch(std::exception const & e)
	{
		std::cerr << e.what() << std::endl;
	}
	

	

	server.closeSocket ();
	return 0;
}
