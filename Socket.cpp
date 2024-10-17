#include "Socket.hpp"

Socket::Socket(int port) : _num_clients(0),_port(port) {
	_client_fd[0] = -1;
	_client_fd[1] = -1;
	createSocket();
	setAddress();
}

void Socket::createSocket() {
	_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket_fd == -1)
		throw std::runtime_error("Failed to create socket");
}

void Socket::setAddress() {
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
}



void Socket::sendMessage(std::string message, int index) {
	send(_client_fd[index], message.c_str(), message.size(), 0);
}
void Socket::receiveMessage(int index) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	
	recv(_client_fd[index], buffer, sizeof(buffer), 0);
	_message[index] = buffer;
}

void Socket::showReceivedMessage(int index) {
	std::cout << _message[index] << std::endl;
}

void Socket::closeSocket() {
	closeClientSocket();
	close(_socket_fd);
}

std::string Socket::getMessage(int index) const{
	return _message[index];
}

void Socket::closeClientSocket() {
	if (_client_fd[0] != -1 && _client_fd[0] != _socket_fd)
		close(_client_fd[0]);
	if (_client_fd[1] != -1 && _client_fd[1] != _socket_fd)
		close(_client_fd[1]);
}

int Socket::getLastClientAdded() const {
	if (_num_clients == 0)
		return -1;
	else
		return _client_fd[_num_clients - 1];
}

int Socket::getClient(int index) const {
	return _client_fd[index];
}

int Socket::getNumClients() const {
	return _num_clients;
}