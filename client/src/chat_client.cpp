#include "chat_client.hpp"
#include "spdlog/fmt/fmt.h"

ChatClient::ChatClient(const std::string &server_ip, const int &port) : server_ip_(server_ip), port_(port) {}

bool ChatClient::connectToServer()
{
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0)
    {
        handleError("Fail to create the server socket");
        return false;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
    server_addr_.sin_port = htons(port_);

    if (connect(server_socket_, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_)) == -1)
    {
        perror("Failed to connect to server");
        close(server_socket_);
        return false;
    }

    return true;
}

void ChatClient::start()
{
    is_running_ = true;

    recv_thread_ = std::thread(&ChatClient::recvMessages, this);
    send_thread_ = std::thread(&ChatClient::sendMessage, this);

    recv_thread_.join();
    send_thread_.join();
}

void ChatClient::recvMessages()
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    while (is_running_)
    {
        ssize_t bytes_recv = recv(server_socket_, buffer, sizeof(buffer), 0);
        if (bytes_recv < 0)
        {
            handleError("Fail to receive message from the server");
            break;
        }
        else if (bytes_recv == 0)
        {
            handleError("Disconnected from the server");
            break;
        }
        else
        {
            fmt::print("Received: {}\n", std::string(buffer, sizeof(buffer)));
        }
        memset(buffer, 0, sizeof(buffer));
    }
}

void ChatClient::sendMessage()
{
    std::string message;

    while (is_running_)
    {
        std::getline(std::cin, message);
        if (message == "/quit")
        {
            break;
        }

        ssize_t byte_sent = send(server_socket_, message.c_str(), message.size(), 0);
        if (byte_sent == -1)
        {
            handleError("Failed to send message");
        }
    }
}

void ChatClient::handleError(const std::string &error_message)
{
    fmt::print("runtime error: {}\n", error_message);
    throw std::runtime_error(error_message);
}
