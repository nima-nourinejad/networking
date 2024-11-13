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

std::string findPath (std::string const & method, std::string const & uri)
{
	std::string path;
	if (method == "GET" && uri == "/")
		path = "html/index.html";
	else if (method == "GET" && uri == "/about")
		path = "html/about.html";
	else if (method == "GET" && uri == "/long")
		path = "html/long.html";
	else if (method == "GET" && uri == "/400")
		path = "html/400.html";
	else if (method == "GET" && uri == "/500")
		path = "html/500.html";
	else
		path = "html/404.html";
	return path;
}

std::string createStatusLine (std::string const & method, std::string const & uri)
{
	std::string statusLine;
	if (method == "GET" && (uri == "/" || uri == "/about" || uri == "/long"))
		statusLine = "HTTP/1.1 200 OK\r\n";
	else if (method == "GET" && (uri == "/500"))
		statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
	else if (method == "GET" && (uri == "/400"))
		statusLine = "HTTP/1.1 400 Bad Request\r\n";
	else
		statusLine = "HTTP/1.1 404 Not Found\r\n";
	return statusLine;
}

std::string requestURI (std::string const & request)
{
	std::istringstream stream (request);
	std::string method;
	std::string uri;
	stream >> method >> uri;
	return uri;
}

std::string requestmethod (std::string const & request)
{
	std::istringstream stream (request);
	std::string method;
	stream >> method;
	return method;
}

std::string readFile (std::string const & path)
{
	std::ifstream file (path.c_str ());
	if (!file.is_open ())
		throw SocketException ("Failed to open file");
	std::stringstream read;
	read << file.rdbuf ();
	file.close ();
	return read.str ();
}

void ClientConnection::sendServiceUnavailable(int socket_fd, size_t maxBodySize)
{
	int temp_fd = accept(socket_fd, nullptr, nullptr);
	if (temp_fd == -1)
		return;

	std::string statusLine = "HTTP/1.1 503 Service Unavailable\r\n";
	std::string contentType = "Content-Type: text/html\r\n";
	std::string connection = "Connection: close\r\n";
	std::string body = readFile ("html/503.html");
	std::string header;
	std::vector<std::string> response;
	
	if (body.size () > maxBodySize)
	{
		std::string transferEncoding = "Transfer-Encoding: chunked\r\n";
		header = statusLine + contentType + transferEncoding + connection;
		response.push_back (header + "\r\n");
		size_t chunkSize;
		std::string chunk;
		std::stringstream sstream;
		while (body.size () > 0)
		{
			chunkSize = std::min (body.size (), maxBodySize);
			chunk = body.substr (0, chunkSize);
			sstream.str ("");
			sstream << std::hex << chunkSize << "\r\n";
			sstream << chunk << "\r\n";
			response.push_back (sstream.str ());
			body = body.substr (chunkSize);
		}
		response.push_back ("0\r\n\r\n");
	}
	else
	{
		std::string contentLength = "Content-Length: " + std::to_string (body.size ()) + "\r\n";
		header = statusLine + contentType + contentLength + connection;
		response.push_back (header + "\r\n" + body);
	}

	while (response.size () > 0)
	{
		ssize_t bytes_sent;
		bytes_sent = send (temp_fd, response[0].c_str (), response[0].size (), MSG_DONTWAIT);
		if (bytes_sent <= 0)
			break;
		if (bytes_sent < static_cast<ssize_t> (response[0].size ()))
		{
			std::string remainPart = response[0].substr (bytes_sent);
			response[0] = remainPart;
		}
		else
			response.erase (response.begin ());
	}
	close (temp_fd);

}

void ClientConnection::sendServerError (int fd, size_t maxBodySize)
{
	std::string statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
	std::string contentType = "Content-Type: text/html\r\n";
	std::string connection = "Connection: close\r\n";
	std::string body = readFile ("html/500.html");
	std::string header;
	std::vector<std::string> response;
	
	if (body.size () > maxBodySize)
	{
		std::string transferEncoding = "Transfer-Encoding: chunked\r\n";
		header = statusLine + contentType + transferEncoding + connection;
		response.push_back (header + "\r\n");
		size_t chunkSize;
		std::string chunk;
		std::stringstream sstream;
		while (body.size () > 0)
		{
			chunkSize = std::min (body.size (), maxBodySize);
			chunk = body.substr (0, chunkSize);
			sstream.str ("");
			sstream << std::hex << chunkSize << "\r\n";
			sstream << chunk << "\r\n";
			response.push_back (sstream.str ());
			body = body.substr (chunkSize);
		}
		response.push_back ("0\r\n\r\n");
	}
	else
	{
		std::string contentLength = "Content-Length: " + std::to_string (body.size ()) + "\r\n";
		header = statusLine + contentType + contentLength + connection;
		response.push_back (header + "\r\n" + body);
	}

	while (response.size () > 0)
	{
		ssize_t bytes_sent;
		bytes_sent = send (fd, response[0].c_str (), response[0].size (), MSG_DONTWAIT);
		if (bytes_sent <= 0)
			break;
		if (bytes_sent < static_cast<ssize_t> (response[0].size ()))
		{
			std::string remainPart = response[0].substr (bytes_sent);
			response[0] = remainPart;
		}
		else
			response.erase (response.begin ());
	}
	close (fd);
}


void ClientConnection::createResponseParts ()
{
	status = PREPARINGRESPONSE;
	connectionType ();
	std::cout << "Creating response for client " << index + 1 << std::endl;
	std::string method = requestmethod (request);
	std::string uri = requestURI (request);
	std::string path = findPath (method, uri);
	std::string body = readFile (path);

	std::string statusLine = createStatusLine (method, uri);

	std::string contentType = "Content-Type: text/html\r\n";
	std::string connection;
	if (keepAlive)
		connection = "Connection: keep-alive\r\n";
	else
		connection = "Connection: close\r\n";

	std::string header;
	if (body.size () > maxBodySize)
	{
		std::string transferEncoding = "Transfer-Encoding: chunked\r\n";
		header = statusLine + contentType + transferEncoding + connection;
		responseParts.push_back (header + "\r\n");
		size_t chunkSize;
		std::string chunk;
		std::stringstream sstream;
		while (body.size () > 0)
		{
			chunkSize = std::min (body.size (), maxBodySize);
			chunk = body.substr (0, chunkSize);
			sstream.str ("");
			sstream << std::hex << chunkSize << "\r\n";
			sstream << chunk << "\r\n";
			responseParts.push_back (sstream.str ());
			body = body.substr (chunkSize);
		}
		responseParts.push_back ("0\r\n\r\n");
	}
	else
	{
		std::string contentLength = "Content-Length: " + std::to_string (body.size ()) + "\r\n";
		header = statusLine + contentType + contentLength + connection;
		responseParts.push_back (header + "\r\n" + body);
	}
	status = READYTOSEND;
	std::cout << "Response created for client " << index + 1 << std::endl;
}

time_t getCurrentTime ()
{
	time_t current_time = time (nullptr);
	if (current_time == -1)
		throw SocketException ("Failed to get current time");
	return current_time;
}

time_t ClientConnection::getPassedTime () const
{
	time_t current_time = getCurrentTime ();
	if (current_time == -1)
		throw SocketException ("Failed to get passed time");
	return (difftime (current_time, connectTime));
}

void ClientConnection::setCurrentTime ()
{
	connectTime = getCurrentTime ();
}