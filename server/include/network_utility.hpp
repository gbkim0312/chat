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
    void joinAllClientThread(std::vector<std::thread> &client_threads);
    void closeAllClientSockets(std::set<int> &client_sockets, std::mutex &client_mutex);
    int acceptClient(int server_socket);
    void removeWhitespaces(std::string &text);
}
