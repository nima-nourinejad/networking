#include "Configration.hpp"

Configration::Configration (int port, std::string const & host, size_t maxBodySize)
    : port (port), host (host), maxBodySize (maxBodySize){};