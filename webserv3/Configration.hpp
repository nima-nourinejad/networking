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
	Configration (int port, std::string const & host, size_t maxBodySize);
};

#endif