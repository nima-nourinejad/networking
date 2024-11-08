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

bool ClientConnection::finishedReceivingNonChunked ()
{
	size_t contentLength;
	std::string contentLengthString;
	if (request.find ("Content-Length: ") == std::string::npos)
	{
		std::cout << "No content length found" << std::endl;
		return true;
	}
	contentLengthString = request.substr (request.find ("Content-Length: ") + 16);
	if (contentLengthString.find ("\r\n") == std::string::npos)
	{
		std::cerr << "Failed to find end of content length" << std::endl;
		changeRequestToBadRequest ();
		return true;
	}
	contentLengthString = contentLengthString.substr (0, contentLengthString.find ("\r\n"));
	try
	{
		contentLength = std::stoul (contentLengthString);
	}
	catch (...)
	{
		std::cerr << "Failed to convert content length to number" << std::endl;
		changeRequestToBadRequest ();
		return true;
	}
	if (receivedLength () > contentLength)
	{
		std::cerr << "Received more data than expected" << std::endl;
		changeRequestToBadRequest ();
		return true;
	}
	if (receivedLength () == contentLength)
		return true;
	return false;
}

bool ClientConnection::finishedReceivingChunked ()
{
	if (request.find ("\r\n0\r\n\r\n") != std::string::npos)
		return true;
	return false;
}

bool ClientConnection::finishedReceiving ()
{
	if (status == RECEIVINGUNKOWNTYPE)
		return false;
	else if (status == RECEIVINGCHUNKED)
		return finishedReceivingChunked ();
	else
		return finishedReceivingNonChunked ();
}

size_t ClientConnection::receivedLength () const
{
	size_t headerLength = request.find ("\r\n\r\n") + 4;
	size_t receivedLength = request.size () - headerLength;
	return receivedLength;
}

void ClientConnection::findRequestType ()
{
	if (request.find ("\r\n\r\n") == std::string::npos)
	{
		if (request.size () > MAX_HEADER_SIZE)
		{
			std::cout << "Header size exceeded the limit" << std::endl;
			changeRequestToBadRequest ();
		}
	}
	else
	{
		if (request.find ("Transfer-Encoding: chunked") != std::string::npos)
			status = RECEIVINGCHUNKED;
		else
			status = RECEIVINGNONCHUNKED;
	}
}

void ClientConnection::connectionType ()
{
	if (request.find ("Connection: close") != std::string::npos)
		keepAlive = false;
}

size_t ClientConnection::getChunkedSize (std::string & unProcessed)
{
	size_t chunkedSize;
	std::string sizeString;
	sizeString = unProcessed.substr (0, unProcessed.find ("\r\n"));
	unProcessed = unProcessed.substr (unProcessed.find ("\r\n") + 2);
	try
	{
		chunkedSize = std::stoul (sizeString, nullptr, 16);
	}
	catch (...)
	{
		changeRequestToBadRequest();
		return 0;
	}
	if (unProcessed.size () < chunkedSize + 2)
	{
		changeRequestToBadRequest();
		return 0;
	}
	return chunkedSize;
}

void ClientConnection::grabChunkedData (std::string & unProcessed, size_t chunkedSize)
{
	std::string data;
	data = unProcessed.substr (0, chunkedSize);
	request += data;
	unProcessed = unProcessed.substr (chunkedSize + 2);
}

void ClientConnection::grabChunkedHeader (std::string & unProcessed, std::string & header)
{
	header = unProcessed.substr (0, unProcessed.find ("\r\n\r\n") + 4);
	unProcessed = unProcessed.substr (unProcessed.find ("\r\n\r\n") + 4);
	request = header;
}

void ClientConnection::handleChunkedEncoding ()
{
	std::string unProcessed = request;
	request.clear ();
	if (unProcessed.find ("Transfer-Encoding: chunked") == std::string::npos)
		return (changeRequestToServerError ());
	if (unProcessed.find ("\r\n0\r\n\r\n") != std::string::npos)
		return (changeRequestToBadRequest ());
	std::string header = "";
	grabChunkedHeader (unProcessed, header);

	size_t chunkedSize;
	while (true)
	{
		if (unProcessed.find ("\r\n") == std::string::npos)
			return (changeRequestToBadRequest ());
		chunkedSize = getChunkedSize (unProcessed);
		if (chunkedSize == 0)
			return;
		else
			grabChunkedData (unProcessed, chunkedSize);
	}
}