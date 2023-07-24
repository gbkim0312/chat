#include "chat_server.hpp"
#include "client.hpp"
#include "chat_room_manager.hpp"
#include "network_utility.hpp"
#include <fmt/core.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <utility>
#include <mutex>
#include <thread>
#include <set>
#include <vector>
#include <memory>
#include <sys/_endian.h> //htons
#include <client_handler.hpp>

namespace
{

    std::string getClientUserName(int client_socket)
    {
        std::string username = network::recvMessageFromClient(client_socket);
        if (username.empty())
        {
            fmt::print("Failed to receive message");
        }
        username.erase(0, username.find_first_not_of(" \t\r\n"));
        username.erase(username.find_last_not_of(" \t\r\n") + 1);

        return username;
    }

    Client createClient(int client_socket, const std::string &username)
    {
        Client client;
        client.socket = client_socket;
        client.state = ClientState::CONNECTED;
        client.username = username;
        return client;
    }

};

class ChatServer::ChatServerImpl
{
public:
    explicit ChatServerImpl(int port) : port_(port) {}

    void start()
    {
        server_state_ = ServerState::RUNNING;

        configServer();
        admin_.username = "ADMIN";

        while (server_state_ == ServerState::RUNNING)
        {
            // accept clients
            int client_socket = acceptClient();

            // get username from the client
            std::string username = getClientUserName(client_socket);
            fmt::print("Client connected. Socket FD: {} | username: {}\n", client_socket, username);

            // insert the client socket to clientSockets.
            client_sockets_.insert(client_socket);

            // create client object
            Client client = createClient(client_socket, username);
            client.state = ClientState::CONNECTED;

            // start clientHandler thread
            std::thread client_thread(&ChatServerImpl::handleClient, this, client);
            client_thread.detach();
            client_threads_.push_back(std::move(client_thread));
        }
    }

    void stop()
    {
        server_state_ = ServerState::STOP;
        joinAllClientThread();
        closeAllClientSockets();
        close(server_socket_);
    }

    ServerState getState() const
    {
        return server_state_;
    }

    void setPort(int port)
    {
        port_ = port;
    }

private:
    // create the server socket and bind with address structure
    void configServer()
    {
        // create the server socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0)
        {
            fmt::println("Fail to create the server socket");
            server_state_ = ServerState::ERROR;
        };

        // set the address of the socket
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = INADDR_ANY;
        server_addr_.sin_port = htons(port_);

        // binding the socket and the address
        if (bind(server_socket_, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_)) == -1)
        {
            fmt::println("Failed to bind socket");
            server_state_ = ServerState::ERROR;
        }

        // listen clients
        if (listen(server_socket_, 1) == -1)
        {
            fmt::println("Fail to listen for connection");
            server_state_ = ServerState::ERROR;
        }

        if (server_state_ == ServerState::RUNNING)
        {
            fmt::println("Server started. Listening on port {}", port_);
        }

        // room_manager_.createDefaultRooms(3);
    }

    void handleClient(Client client)
    {

        const ClientTrigger trigger = ClientTrigger::SEND_ROOMS; // 초기 트리거 설정
        ClientHandler handler(trigger);

        while (server_state_ == ServerState::RUNNING || client.state != ClientState::DEFAULT)
        {
            const ClientState newState = handler.onClientTrigger(client, room_manager_);

            client.state = newState;
        }
        client_sockets_.erase(client.socket);
        close(client.socket);
        fmt::print("client {} (socket: {}) disconnected", client.username, client.socket);
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
        const std::lock_guard<std::mutex> lock_guard(client_mutex_);
        for (auto client_socket : client_sockets_)
        {
            close(client_socket);
        }
        client_sockets_.clear();
    }

    int acceptClient() const
    {
        const int client_socket = accept(server_socket_, nullptr, nullptr);
        if (client_socket < 0)
        {
            fmt::print("Failed to accept client connection");
        }
        return client_socket;
    }

    sockaddr_in server_addr_{};
    int port_ = 0;
    int server_socket_ = 0;
    std::set<int> client_sockets_;
    bool is_running_ = false;
    std::mutex client_mutex_;
    ServerState server_state_ = ServerState::STOP;
    std::vector<std::thread> client_threads_;
    int client_id_ = 0;
    ChatRoomManager room_manager_;
    Client admin_;
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

void ChatServer::setPort(int port)
{
    pimpl_->setPort(port);
}