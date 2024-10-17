#include "Client.hpp"
#include "Server.hpp"
#include <sys/epoll.h>


// int main() {
// 	Server server(9001);
// 	server.connectToSocket();
// 	Client client1(9001);
// 	client1.connectToSocket();
// 	Client client2(9001);
// 	client2.connectToSocket();
// 	int charCount[2] = {0, 0};

// 	struct epoll_event event[5];
// 	struct epoll_event eventQueue[15];
// 	int epoll_fd = epoll_create1(0);
// 	client1.sendMessage("Big", 0);
// 	client2.sendMessage("Small", 0);
// 	server.getNewClient();
// 	server.getNewClient();
	
// 	///////////Above done via two tabs of browser
// 	server.receiveMessage(0);
// 	server.receiveMessage(1);
// 	////////////////////////
// 	int fd[2];
// 	fd1 = open("big.txt", O_RDONLY);
// 	fd2 = open("small.txt", O_RDONLY);



// 	server.getNewClient();
// 	event[0].events = EPOLLIN;
// 	event[0].data.fd = fd1;
// 	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd1, &event[0]);
	
	
// 	server.getNewClient();
// 	event[1].events = EPOLLIN;
// 	event[1].data.fd = fd2;
// 	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd2, &event[1]);
	
// 	// server.getNewClient();
// 	// event[0].events = EPOLLIN;
// 	// event[0].data.fd = server.getLastClientAdded();
// 	// epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.getLastClientAdded(), &event[0]);
	
	
// 	// server.getNewClient();
// 	// event[1].events = EPOLLIN;
// 	// event[1].data.fd = server.getLastClientAdded();
// 	// epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.getLastClientAdded(), &event[1]);





// 	int num_events = epoll_wait(epoll_fd, eventQueue, 15, -1);
// 	for (int i = 0; i < num_events; ++i)
// 	{
// 		if (eventQueue[i].data.fd == server.getClient(i))
// 		{

// 			server.sendMessage( "I read the requested file and the number of characters is " + charCount[i], i);
// 		}
// 	}








// 				// if (i == 0)
// 				// {
// 				// 	usleep(10000000);
// 				// 	server.receiveMessage(i);
// 				// 	server.showReceivedMessage(i);
// 				// 	server.sendMessage( "I recieved this: " + server.getMessage(i), i);
// 				// 	client1.receiveMessage(0);
// 				// 	client1.showReceivedMessage(0);
// 				// }
// 				// else
// 				// {
// 				// 	server.receiveMessage(i);
// 				// 	server.showReceivedMessage(i);
// 				// 	server.sendMessage( "I recieved this: " + server.getMessage(i), i);
// 				// 	client2.receiveMessage(0);
// 				// 	client2.showReceivedMessage(0);
// 				// }
// 				// server.receiveMessage(i);
// 				// server.showReceivedMessage(i);
// 				// server.sendMessage( "I recieved this: " + server.getMessage(i), i);
// 		// 	}
// 		// }
		
// 	client2.receiveMessage(0);
// 	client2.showReceivedMessage(0);
// 	client1.receiveMessage(0);
// 	client1.showReceivedMessage(0);
	
	
// 	client1.closeSocket();
// 	client2.closeSocket();
// 	server.closeSocket();
// 	close(epoll_fd);
// }


// int main() {
// 	Server server(9001);
// 	server.connectToSocket();
// 	Client client1(9001);
// 	client1.connectToSocket();
// 	Client client2(9001);
// 	client2.connectToSocket();
// 	int charCount[2] = {0, 0};

// 	struct epoll_event event[5];
// 	struct epoll_event eventQueue[15];
// 	int epoll_fd = epoll_create1(0);
// 	client1.sendMessage("Big", 0);
// 	client2.sendMessage("Small", 0);
// 	server.getNewClient();
// 	server.getNewClient();
	
// 	///////////Above done via two tabs of browser
// 	server.receiveMessage(0);
// 	server.receiveMessage(1);
// 	////////////////////////
// 	int fd[2];
// 	fd1 = open("big.txt", O_RDONLY);
// 	fd2 = open("small.txt", O_RDONLY);



// 	server.getNewClient();
// 	event[0].events = EPOLLIN;
// 	event[0].data.fd = fd1;
// 	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd1, &event[0]);
	
	
// 	server.getNewClient();
// 	event[1].events = EPOLLIN;
// 	event[1].data.fd = fd2;
// 	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd2, &event[1]);
	
// 	////////////////////////some how reading different files at the same time and whenever a file is read, send the number of characters to the client that requested it





// 	int num_events = epoll_wait(epoll_fd, eventQueue, 15, -1);
// 	for (int i = 0; i < num_events; ++i)
// 	{
// 		if (eventQueue[i].data.fd == server.getClient(i))
// 		{
			
// 			server.sendMessage( "I read the requested file and the number of characters is " + charCount[i], i);
// 		}
// 	}
		
// 	client2.receiveMessage(0);
// 	client2.showReceivedMessage(0);
// 	client1.receiveMessage(0);
// 	client1.showReceivedMessage(0);
	
	
// 	client1.closeSocket();
// 	client2.closeSocket();
// 	server.closeSocket();
// 	close(epoll_fd);
// }

int main() {
    // Set up server and clients
    Server server(9001);
    server.connectToSocket();
    Client client1(9001), client2(9001);
    client1.connectToSocket();
    client2.connectToSocket();

    // Simulated request sent from two clients
    client1.sendMessage("Big", 0);
    client2.sendMessage("Small", 0);

    // Server accepts both clients
    server.getNewClient();
    server.getNewClient();

	// ///////simple test
	// server.receiveMessage(0);
	// server.showReceivedMessage(0);
	// server.receiveMessage(1);
	// server.showReceivedMessage(1);
	// client1.closeSocket();
    // client2.closeSocket();
    // server.closeSocket();


    // Preparing for multiplexed IO using epoll
    struct epoll_event event[5], eventQueue[15];
    int epoll_fd = epoll_create1(0);

    // Open files (one large slow file, one smaller fast file)
    int fd1;
	int fd2;
    fd1 = open("small_copy.txt", O_RDONLY);  // Simulates a "big file"
	std::cout << "fd1: " << fd1 << std::endl;
    fd2 = open("small.txt", O_RDONLY);     // Simulates a "small file"

	// int fd[2];
    // fd1 = open("small.txt", O_RDONLY);  // Simulates a "big file"
	// std::cout << "fd1: " << fd1 << std::endl;
    // fd2 = open("/dev/random", O_RDONLY);
	
    if (fd1 == -1 || fd2 == -1) {
        std::cerr << "Error opening files!" << std::endl;
        return 1;
    }

    // Add both file descriptors to epoll for reading
    event[0].events = EPOLLIN | EPOLLET;
    event[0].data.fd = fd1;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd1, &event[0]);

    event[1].events = EPOLLIN | EPOLLET;
    event[1].data.fd = fd2;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd2, &event[1]);

    // Buffer to hold file read data
    char buffer[256];
    int charCount[2] = {0, 0};

    // Event loop: wait for events and handle them
	std::cout << "Waiting for events" << std::endl;
	while (true) {
    int num_events = epoll_wait(epoll_fd, eventQueue, 15, -1);
    std::cout << "num_events: " << num_events << std::endl;

    for (int i = 0; i < num_events; ++i) {
        if (eventQueue[i].data.fd == fd1) {
            std::cout << "Big file read" << std::endl;
            // Reading from the large file (/dev/random)
            int bytesRead = read(fd1, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                charCount[0] += bytesRead;
                std::string message = "I read the requested file and the number of characters is " + std::to_string(charCount[0]);
                server.sendMessage(message, 0); // Send message to client 1
                client1.receiveMessage(0);
                client1.showReceivedMessage(0);
            } else {
                // Handle end-of-file or error if bytesRead <= 0
                std::cout << "No more data to read from the big file." << std::endl;
            }
            std::cout << "We are done reading Big File" << std::endl;
        }
        if (eventQueue[i].data.fd == fd2) {
            std::cout << "**Small file read" << std::endl;
            // Reading from the small file
            int bytesRead = read(fd2, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                charCount[1] += bytesRead;
                std::string message = "I read the requested file and the number of characters is " + std::to_string(charCount[1]);
                server.sendMessage(message, 1);  // Send message to client 2
                client2.receiveMessage(1);
                client2.showReceivedMessage(1);
            } else {
                // Handle end-of-file or error if bytesRead <= 0
                std::cout << "No more data to read from the small file." << std::endl;
               }
        }
    }
}

    // Clean up
    client1.closeSocket();
    client2.closeSocket();
    server.closeSocket();
    close(fd1);
    close(fd2);
    close(epoll_fd);

    return 0;
}
