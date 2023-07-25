#include "network_utility.hpp"
#include <string>
#include <array>
#include <sys/socket.h>

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

    void removeWhitespaces(std::string &text)
    {
        text.erase(0, text.find_first_not_of(" \t\r\n"));
        text.erase(text.find_last_not_of(" \t\r\n") + 1);
    }

}
