#include "SocketException.hpp"

SocketException::SocketException (std::string const & message)
    : std::runtime_error (message + " : " + strerror (errno)){};
void SocketException::log () const
{
	try
	{
		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now ();
		std::time_t timeInSeconds = std::chrono::system_clock::to_time_t (timePoint);
		std::ofstream logFile ("socket_error.log", std::ios::app);
		if (!logFile.is_open ())
		 	throw std::runtime_error ("Failed to open log file");
		logFile << std::put_time (std::localtime (&timeInSeconds), "%Y-%m-%d %H:%M:%S") << " : ";
		logFile << what () << std::endl;
		logFile.close ();
	}
	catch (std::exception const & e)
	{
		std::cerr << "Failed to log exception : " << e.what () << std::endl;
		std::cerr << "Original exception : " << what () << std::endl;
	}
}