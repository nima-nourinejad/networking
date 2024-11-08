#include "Server.hpp"
#include <sys/epoll.h>

int main ()
{
	std::map<std::string, std::string> routes;
	routes["/"] = "index.html";
	routes["/about"] = "html/about.html";
	routes["/long"] = "html/long.html";
	routes["/400"] = "html/400.html";
	routes["/404"] = "html/404.html";
	routes["/500"] = "html/500.html";

	
	Server server(9001, "127.0.0.3", 1, routes);
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
