#include "Client.hpp"
#include "Server.hpp"

int main() {
	Server server(8080);
	Client client(8080);
	server.connectToSocket();
	client.connectToSocket();
	server.getNewClient();
	server.sendMessage("Hello, client!");
	client.receiveMessage();
	client.showReceivedMessage();
	// server.closeSocket();
	// client.closeSocket();
}