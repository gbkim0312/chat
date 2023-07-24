#pragma once
#include <string>

namespace network
{
    constexpr int BUFFER_SIZE = 1024;

    std::string recvMessageFromClient(int client_socket);
    bool sendMessageToClient(int socket, const std::string &message);
}