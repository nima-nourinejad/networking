#include "Client.hpp"

Client::Client(int port) : Socket(port) {};
void Client::connectToSocket() {
	if (connect(_socket_fd, (struct sockaddr *) &_address, sizeof(_address) == -1))
		throw std::runtime_error("Failed to connect to socket");
};

void Client::receiveMessage() {
	char buffer[1024];
	recv(_socket_fd, buffer, sizeof(buffer), NORMAL);
	_message = buffer;
}

void Client::showReceivedMessage() {
	std::cout << _message << std::endl;
}