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
            fmt::println("* [ERROR] Failed to receive username");
        }
        network::removeWhitespaces(username);

        return username;
    }

    network::Client createClient(int client_socket, const std::string &username)
    {
        network::Client client;
        client.socket = client_socket;
        client.state = network::ClientState::CONNECTED;
        client.username = username;
        return client;
    }

};
namespace network
{
    class ChatServer::ChatServerImpl
    {
    public:
        explicit ChatServerImpl(int port) : port_(port) {}

        void start()
        {
            server_state_ = ServerState::RUNNING;

            configServer();

            while (server_state_ == ServerState::RUNNING)
            {
                // accept clients
                int client_socket = acceptClient(server_socket_);

                // get username from the client
                std::string username = getClientUserName(client_socket);
                fmt::println("* Client connected. Socket FD: {} | username: {}", client_socket, username);

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
            joinAllClientThread(client_threads_);
            closeAllClientSockets(client_sockets_, client_mutex_);
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
                fmt::println("* [ERROR] Fail to create the server socket");
                server_state_ = ServerState::ERROR;
            };

            // set the address of the socket
            server_addr_.sin_family = AF_INET;
            server_addr_.sin_addr.s_addr = INADDR_ANY;
            server_addr_.sin_port = htons(port_);

            // binding the socket and the address
            if (bind(server_socket_, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_)) == -1)
            {
                fmt::println("* [ERROR] Failed to bind socket");
                server_state_ = ServerState::ERROR;
            }
            // listen clients
            if (listen(server_socket_, 1) == -1)
            {
                fmt::println("* [ERROR] Fail to listen for connection");
                server_state_ = ServerState::ERROR;
            }
            if (server_state_ == ServerState::RUNNING)
            {
                room_manager_.createDefaultRooms(3);
                fmt::println("* Server started. Listening on port {}", port_);
            }
        }

        void handleClient(Client client)
        {
            const ClientTrigger trigger = ClientTrigger::SEND_ROOMS; // 초기 트리거 설정
            ClientHandler handler(trigger, room_manager_, client);

            while (server_state_ == ServerState::RUNNING && client.state != ClientState::DEFAULT)
            {
                const ClientState newState = handler.onClientTrigger();
                client.state = newState;
            }

            client_sockets_.erase(client.socket);
            close(client.socket);
            fmt::println("* client {} (socket: {}) disconnected", client.username, client.socket);
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
}