#include "ClientConnection.hpp"

ClientConnection::ClientConnection ()
    : index (-1), fd (-1), status (DISCONNECTED), keepAlive (true){};