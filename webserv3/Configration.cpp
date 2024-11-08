#include "Configration.hpp"

Configration::Configration (int port, std::string const & host, size_t maxBodySize, std::map<std::string, std::string> const & routes)
    : port (port), host (host), maxBodySize (maxBodySize), routes (routes){};