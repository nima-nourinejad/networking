#include "Server.hpp"
#include <sys/epoll.h>

int main ()
{
	std::map<std::string, std::string> routes;
	routes["/"] = "index.html";
	routes["/about"] = "about.html";
	routes["/delay"] = "delay.html";
	routes["404"] = "404.html";
	Server server (9001, "127.0.0.3", routes);
	server.connectToSocket ();

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
