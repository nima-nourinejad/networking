#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <map>
#include <string>

class Configration
{
		public:
	int port;
	std::string host;
	size_t maxBodySize;
	std::map<std::string, std::string> routes;
	Configration (int port, std::string const & host, size_t maxBodySize, std::map<std::string, std::string> const & routes);
};

#endif