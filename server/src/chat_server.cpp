#include "chat_server.hpp"
#include "spdlog/fmt/fmt.h"

ChatServer::ChatServer(const int &port) : port_(port) {}

ServerState ChatServer::start()
{
    // create the server socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0)
    {
        handleError("Fail to create the server socket");
        return ServerState::ERROR;
    };

    // set the address of the socket
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(port_);

    // binding the socket and the address
    if (bind(server_socket_, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_)) == -1)
    {
        handleError("Failed to bind socket");
        return ServerState::ERROR;
    }

    // listen clients
    if (listen(server_socket_, 1) == -1)
    {
        handleError("Fail to listen for connection");
        return ServerState::ERROR;
    }

    is_running_ = true;
    fmt::print("Server started. Listening on port {} \n", port_);

    while (is_running_)
    {
        // accept clients
        int clinet_socket = accept(server_socket_, nullptr, nullptr);
        if (clinet_socket < 0)
        {
            handleError("Failed to accept clinet connection");
            continue;
        }

        fmt::print("Client connected. Socket FD: {} \n", clinet_socket);

        // push_back the client socket to clientSockets.
        client_sockets_.push_back(clinet_socket);

        // start clientHandler thread
        std::thread client_thread(&ChatServer::handleClient, this, clinet_socket);
        client_thread.detach(); // 스레드를 분리해서 백그라운드에서 실행
    }

    is_running_ = false;
    return ServerState::STOP;
}

void ChatServer::handleClient(const int &clinet_socket)
{
    // create buffer for message and clear it
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // receive messages
    while (is_running_)
    {
        ssize_t bytesRecv = recv(clinet_socket, buffer, sizeof(buffer), 0);
        if (bytesRecv < 0)
        {
            handleError("Failed to receive message from the client");
            break;
        }
        else if (bytesRecv == 0)
        {
            fmt::print("Client disconnected. Socket FD: {}", clinet_socket);
            break;
        }
        else
        {
            broadcastMessage(std::string(buffer, bytesRecv), bytesRecv, clinet_socket);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    close(clinet_socket);

    // remove the clientSocket.
    std::lock_guard<std::mutex> lock_guard(client_mutex_);
    client_sockets_.erase(std::remove(client_sockets_.begin(), client_sockets_.end(), client_sockets_), client_sockets_.end());
}

void ChatServer::broadcastMessage(const std::string &message, const ssize_t &message_size, const int &sender_socket)
{
    for (int client_socket : client_sockets_)
    {
        if (client_socket != sender_socket)
        {
            ssize_t sendBytes = send(client_socket, message.c_str(), message_size, 0);
            if (sendBytes < 0)
            {
                handleError("Failed to send message to clients");
            }
        }
    }
}

void ChatServer::handleError(const std::string &error_message)
{
    fmt::print("runtime error: {}\n", error_message);
    // throw std::runtime_error(errorMessage);
}