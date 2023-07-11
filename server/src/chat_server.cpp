#include "chat_server.hpp"
#include "spdlog/fmt/fmt.h"
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct client
{
    uint16_t client_id;
    std::string username;
    int client_socket;
};

class ChatServer::ChatServerImpl
{
public:
    ChatServerImpl(int port) : port_(port) {}

    void start()
    {
        server_state_ = ServerState::RUNNING;

        // create the server socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0)
        {
            handleError("Fail to create the server socket");
            server_state_ = ServerState::ERROR;
        };

        // set the address of the socket
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = INADDR_ANY;
        server_addr_.sin_port = htons(port_);

        // binding the socket and the address
        if (bind(server_socket_, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_)) == -1)
        {
            handleError("Failed to bind socket");
            server_state_ = ServerState::ERROR;
        }

        // listen clients
        if (listen(server_socket_, 1) == -1)
        {
            handleError("Fail to listen for connection");
            server_state_ = ServerState::ERROR;
        }

        if (server_state_ == ServerState::RUNNING)
        {
            fmt::print("Server started. Listening on port {}\n", port_);
        }

        while (server_state_ == ServerState::RUNNING)
        {
            // accept clients
            int client_socket = accept(server_socket_, nullptr, nullptr);
            if (client_socket < 0)
            {
                handleError("Failed to accept client connection");
                continue;
            }

            fmt::print("Client connected. Socket FD: {} \n", client_socket);

            // push_back the client socket to clientSockets.
            client_sockets_.push_back(client_socket);

            // start clientHandler thread
            std::thread client_thread(&ChatServerImpl::handleClient, this, client_socket);
            client_thread.detach();
            client_threads_.push_back(std::move(client_thread));
        }

        // exit(-1);
        // server_state_ = ServerState::STOP;
    }

    void handleClient(const int &client_socket)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        while (server_state_ == ServerState::RUNNING)
        {
            auto bytesRecv = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytesRecv < 0)
            {
                handleError("Failed to receive message from the client");
                break;
            }
            else if (bytesRecv == 0)
            {
                fmt::print("Client disconnected. Socket FD: {}", client_socket);
                break;
            }

            broadcastMessage(std::string(buffer, bytesRecv), bytesRecv, client_socket);
            memset(buffer, 0, sizeof(buffer));
        }

        close(client_socket);

        // remove the clientSocket.
        std::lock_guard<std::mutex> lock_guard(client_mutex_);
        client_sockets_.erase(std::remove(client_sockets_.begin(), client_sockets_.end(), client_socket), client_sockets_.end());
    }

    void broadcastMessage(const std::string &message, const ssize_t &message_size, const int &sender_socket)
    {
        for (auto client_socket : client_sockets_)
        {
            if (client_socket != sender_socket)
            {
                auto sendBytes = send(client_socket, message.c_str(), message_size, 0);
                if (sendBytes < 0)
                {
                    handleError("Failed to send message to clients");
                }
            }
        }
    }

    void stop()
    {
        server_state_ = ServerState::STOP;
        joinAllClientThread();
        closeAllClientSockets();
        close(server_socket_);
    }

    void handleError(const std::string &error_message)
    {
        fmt::print("runtime error: {}\n", error_message);
        // throw std::runtime_error(errorMessage);
    }

    ServerState getState() const
    {
        return server_state_;
    }

    void joinAllClientThread()
    {
        for (auto &client_thread : client_threads_)
        {
            if (client_thread.joinable())
            {
                client_thread.join();
            }
        }
    }

    void closeAllClientSockets()
    {
        std::lock_guard<std::mutex> lock_guard(client_mutex_);
        for (auto client_socket : client_sockets_)
        {
            close(client_socket);
        }
        client_sockets_.clear();
    }

private:
    sockaddr_in server_addr_{};
    int port_ = 0;
    int server_socket_ = 0;
    std::vector<int> client_sockets_;
    bool is_running_ = false;
    std::mutex client_mutex_;
    ServerState server_state_ = ServerState::STOP;
    std::vector<std::thread> client_threads_;
};

ChatServer::ChatServer(int port) : pimpl_(std::make_unique<ChatServerImpl>(port)) {}

ChatServer::~ChatServer() = default; // 필수적..?

void ChatServer::start()
{
    pimpl_->start();
}

ServerState ChatServer::getState()
{
    return pimpl_->getState();
}

void ChatServer::stop()
{
    pimpl_->stop();
}