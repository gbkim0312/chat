#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <set>
namespace network
{
    constexpr int BUFFER_SIZE = 1024;

    std::string recvMessageFromClient(int client_socket);
    bool sendMessageToClient(int socket, const std::string &message);
    void removeWhitespaces(std::string &text);
}
