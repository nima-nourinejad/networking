#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class Socket
{
      protected:
	int _socket_fd;
	struct sockaddr_in _address;
	const int _port;

      public:
	Socket (int port);
	void setAddress ();
	void createSocket ();

	virtual void connectToSocket () = 0;
	virtual void closeSocket () = 0;
	int getSocketFD () const;
};

#endif