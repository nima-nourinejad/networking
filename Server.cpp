#include "Server.hpp"

Server::Server(int port) : Socket(port) {};

void Server::connectToSocket() {
	if (bind(_socket_fd, (struct sockaddr *) &_address, sizeof(_address)) == -1)
		throw std::runtime_error("Failed to bind socket");
	if (listen(_socket_fd, MAX_CONNECTIONS) == -1)
		throw std::runtime_error("Failed to listen on socket");
}

void Server::getNewClient() {
	_client_fd = accept(_socket_fd, NULL, NULL);
	if (_client_fd == -1)
		throw std::runtime_error("Failed to accept client");
}

void Server::sendMessage(std::string message) {
	send(_client_fd, message.c_str(), message.size(), NORMAL);
}