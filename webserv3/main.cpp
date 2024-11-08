#include "Server.hpp"
#include <sys/epoll.h>

int main ()
{
	Server server(9001, "127.0.0.3", 1);

	try
	{
		while (server.signal_status != SIGINT)
		{
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
