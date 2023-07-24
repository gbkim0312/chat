#include "network_utility.hpp"
#include <string>
#include <array>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <fmt/core.h>
#include <vector>
#include <set>
#include <mutex>

namespace network
{
    std::string recvMessageFromClient(int client_socket)
    {
        std::array<char, BUFFER_SIZE> buffer = {0};
        auto bytes_received = recv(client_socket, buffer.data(), BUFFER_SIZE, 0);
        const std::string message = std::string(buffer.data(), bytes_received);

        return message;
    }
    bool sendMessageToClient(int socket, const std::string &message)
    {
        auto sendBytes = send(socket, message.c_str(), message.size(), 0);
        return (sendBytes > 0);
    }

    void joinAllClientThread(std::vector<std::thread> &client_threads)
    {
        for (auto &client_thread : client_threads)
        {
            if (client_thread.joinable())
            {
                client_thread.join();
            }
        }
    }

    void closeAllClientSockets(std::set<int> &client_sockets, std::mutex &client_mutex)
    {
        const std::lock_guard<std::mutex> lock_guard(client_mutex);
        for (auto client_socket : client_sockets)
        {
            close(client_socket);
        }
        client_sockets.clear();
    }

    int acceptClient(int server_socket)
    {
        const int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0)
        {
            fmt::print("Failed to accept client connection");
        }
        return client_socket;
    }
}

namespace utility
{
    void removeWhitespaces(std::string &text)
    {
        text.erase(0, text.find_first_not_of(" \t\r\n"));
        text.erase(text.find_last_not_of(" \t\r\n") + 1);
    }
}