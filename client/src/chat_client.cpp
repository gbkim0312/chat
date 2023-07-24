#include "chat_client.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <array>
#include <string>
#include <thread>
#include <memory>
#include <utility>

#include <fmt/core.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdexcept>

namespace network
{
    class ChatClient::ChatClientImpl
    {
    public:
        explicit ChatClientImpl(std::string server_ip, int port, std::string username) : server_ip_(std::move(server_ip)), port_(port), username_(std::move(username)) {}

        bool connectToServer()
        {
            server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket_ < 0)
            {
                handleError("Fail to create the server socket");
                return false;
            }

            server_addr_.sin_family = AF_INET;
            server_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
            server_addr_.sin_port = htons(port_); // NOLINT

            if (connect(server_socket_, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_)) == -1)
            {
                fmt::print("Failed to connect to server\n");
                close(server_socket_);
                return false;
            }
            return true;
        }

        void start()
        {
            is_running_ = true;

            // send username to the server
            auto byte_sent = send(server_socket_, username_.c_str(), username_.size(), 0);
            if (byte_sent == -1)
            {
                handleError("Failed to send message");
            }

            recv_thread_ = std::thread(&ChatClientImpl::recvMessages, this);
            send_thread_ = std::thread(&ChatClientImpl::sendMessage, this);

            recv_thread_.join();
            send_thread_.join();
        }

    private:
        static void handleError(const std::string &error_message)
        {
            fmt::print("runtime error: {}\n", error_message);
            throw std::runtime_error(error_message);
        }

        void recvMessages() const
        {

            while (is_running_)
            {
                std::array<char, 1024> buffer = {0};
                auto bytes_recv = recv(server_socket_, buffer.data(), 1024, 0);
                const std::string message = std::string(buffer.data(), bytes_recv);

                if (message.empty())
                {
                    handleError("Disconnected from the server");
                    break;
                }
                else if (message == "LEAVE")
                {
                    fmt::println("Rom Closed");
                    const std::string message = "/back";
                    send(server_socket_, message.c_str(), message.size(), 0);
                }
                else
                {
                    fmt::println("{}", std::string(buffer.data(), bytes_recv));
                }
            }
        }

        void sendMessage() const
        {
            std::string message;

            while (is_running_)
            {
                std::getline(std::cin, message);
                if (message == "/quit")
                {
                    break;
                }

                auto byte_sent = send(server_socket_, message.c_str(), message.size(), 0);
                if (byte_sent == -1)
                {
                    handleError("Failed to send message");
                }
            }
        }

        sockaddr_in server_addr_{};
        std::string server_ip_;
        int port_;
        int server_socket_ = 0;
        bool is_running_ = false;
        std::thread recv_thread_;
        std::thread send_thread_;
        std::string username_;
    };

    ChatClient::ChatClient(const std::string &server_ip, int port, const std::string &username) : pimpl_(std::make_unique<ChatClientImpl>(server_ip, port, username)) {}

    ChatClient::~ChatClient() = default;

    bool ChatClient::connectToServer()
    {
        return pimpl_->connectToServer();
    }

    void ChatClient::start()
    {
        pimpl_->start();
    }
}