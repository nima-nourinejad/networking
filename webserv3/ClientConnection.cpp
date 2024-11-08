#include "ClientConnection.hpp"

ClientConnection::ClientConnection ()
    : index (-1), fd (-1), status (DISCONNECTED), keepAlive (true){};

void ClientConnection::changeRequestToBadRequest ()
{
	request.clear ();
	request = "Get /400 HTTP/1.1\r\n";
	status = RECEIVED;
}

void ClientConnection::changeRequestToServerError ()
{
	request.clear ();
	request = "Get /500 HTTP/1.1\r\n";
	status = RECEIVED;
}