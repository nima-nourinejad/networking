#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


class Client
{
	protected:
		const size_t MAX_HEADER_SIZE = 32768;
		enum State
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
		int index;
		int fd;
		int status;
		bool keepAlive;
		time_t requestTime;
		std::string request;
		std::vector<std::string> responseParts;
		time_t getPassedTime() const;
		time_t getCurrentTime() const;
		void setCurrentTime();
		void handleChunkedEncoding();
		void changeRequestToBadRequest();
		void changeRequestToServerError();
		void grabChunkedHeader(std::string & unProcessed, std::string & header);
		size_t getChunkedSize(std::string & unProcessed);
		void grabChunkedData(std::string & unProcessed, size_t chunkedSize);
		void connectionType();
		bool finishedReceiving();
		bool finishedReceivingChunked();
		bool finishedReceivingNonChunked();
		size_t receivedLength() const;
		void findRequestType();
		void sendResponseParts();
		void receiveMessage();
		void createResponseParts();
		void handleTimeout();

	public:
		Client();
		



};

#endif