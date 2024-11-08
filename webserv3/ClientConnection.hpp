#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

#include <ctime>
#include <string>
#include <vector>

enum ClientStatus
{
	DISCONNECTED,
	WAITFORREQUEST,
	RECEIVINGUNKOWNTYPE,
	RECEIVINGNONCHUNKED,
	RECEIVINGCHUNKED,
	RECEIVED,
	PREPARINGRESPONSE,
	READYTOSEND
};

class ClientConnection
{
      public:
	int index;
	int fd;
	int status;
	bool keepAlive;
	time_t connectTime;
	std::string request;
	std::vector<std::string> responseParts;
	ClientConnection ();
};

#endif