#include "Server.hpp"
#include <sys/epoll.h>

int main ()
{
	Server server (9001);
	server.customSignal ();
	server.makeSocketNonBlocking ();
	server.connectToSocket ();

	std::string message;
	while (server.signal_status != SIGINT)
	{
		server.acceptClient ();
		server.handleEvents ();
	}

	server.closeSocket ();
	return 0;
}
