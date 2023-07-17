#include "chat_server.hpp"
#include "client.hpp"
#include "chat_room.hpp"
#include "chat_room_manager.hpp"
#include <fmt/core.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <array>
#include <string>
#include <utility>
#include <mutex>
#include <thread>
#include <set>
#include <vector>
#include <memory>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <sys/_endian.h> //htons

namespace
{
    constexpr int BUFFER_SIZE = 1024;
};

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

        // create default rooms
        // TODO: User가 room을 만들 수 있도록 해야함
        room_manager_.createRoom("Room1", 0);
        room_manager_.createRoom("Room2", 1);
        room_manager_.createRoom("Room3", 2);
    }

    // Handle each client independently according to their state
    void handleClient(Client client)
    {
        while (server_state_ == ServerState::RUNNING)
        {
            const ClientState client_state = client.state;

            switch (client_state)
            {
            case ClientState::CONNECTED:
                sendRoomLists(client);
                break;
            case ClientState::ROOM_LIST_SENT:
                recvSelectedRoom(client);
                break;
            case ClientState::ROOM_SELECTED:
                joinRoom(client);
                break;
            case ClientState::CREATING_ROOM:
                createNewRoom(client);
                break;
            case ClientState::CHATTING:
                startChatting(client);
                break;
            case ClientState::LEAVING:
                leaveRoom(client);
                break;
            default:
                break;
            }
        }
    }

    static void handleError(const std::string &error_message)
    {
        fmt::print("runtime error: {}\n", error_message);
        // throw std::runtime_error(errorMessage);
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

    static std::string getClientUserName(int client_socket)
    {
        std::array<char, 1024> buffer = {0};

        auto bytes_recv = recv(client_socket, buffer.data(), 1024, 0);
        if (bytes_recv == -1)
        {
            handleError("Failed to receive message");
        }

        std::string username = std::string(buffer.data(), bytes_recv);

        username.erase(0, username.find_first_not_of(" \t\r\n"));
        username.erase(username.find_last_not_of(" \t\r\n") + 1);

        return username;
    }

    Client createClient(int client_socket, const std::string &username)
    {
        Client client;
        client.id = ++client_id_;
        client.socket = client_socket;
        client.state = ClientState::CONNECTED;
        client.username = username;
        return client;
    }

    bool sendRoomLists(Client &client)
    {
        auto rooms = room_manager_.getRooms();

        // TODO: 비어있을 때 처리 추가 필요
        if (rooms.empty())
        {
            fmt::print("room is empty\n");
            client.state = ClientState::CONNECTED;
            return false;
        };

        // sort rooms by index
        std::sort(rooms.begin(), rooms.end(), [](ChatRoom const &r1, ChatRoom const &r2)
                  { return r1.getIndex() < r2.getIndex(); });

        // build string
        std::string room_lists_string = "Choose Room from the following List\n(/create to create new room)\n\n";
        for (const auto &room : rooms)
        {
            room_lists_string += fmt::format("{} | Name: {}\n", room.getIndex(), room.getName());
        }

        // send room lists
        if (sendMessageToClient(client.socket, room_lists_string))
        {
            client.state = ClientState::ROOM_LIST_SENT;
            return true;
        }
        // TODO: 예외처리 추가 필요
        else
        {
            handleError("Fail to send Room Lists");
            client.state = ClientState::CONNECTED;
            return false;
        }
    }

    static void recvSelectedRoom(Client &client)
    {
        std::array<char, BUFFER_SIZE> buffer = {0};
        auto bytes_received = recv(client.socket, buffer.data(), BUFFER_SIZE, 0);
        const std::string message = std::string(buffer.data(), bytes_received);

        client.state = ClientState::ROOM_SELECTED;

        if (bytes_received < 0 || message.empty())
        {
            fmt::print("Client {} is disconnected\n", client.username);
            client.state = ClientState::DEFAULT;
        }
        else if (message == "/create")
        {
            client.state = ClientState::CREATING_ROOM;
        }
        else if (message == "/reload")
        {
            client.state = ClientState::CONNECTED;
        }
        else
        {
            try
            {
                client.room_index = std::stoi(message);
            }
            catch (std::invalid_argument &e)
            {
                sendMessageToClient(client.socket, "Wrong input. Choose again");
                client.state = ClientState::ROOM_LIST_SENT;
            }
        }
    }

    void createNewRoom(Client &client)
    {
        sendMessageToClient(client.socket, "Input room name");

        std::array<char, BUFFER_SIZE> buffer = {0};
        auto bytes_received = recv(client.socket, buffer.data(), BUFFER_SIZE, 0);
        const std::string message = std::string(buffer.data(), bytes_received);

        if (message.empty())
        {
            fmt::print("client {} is disconnected\n", client.username);
            client.state = ClientState::DEFAULT;
        }
        else
        {
            room_manager_.createRoom(message, static_cast<int>(room_manager_.getRooms().size()));
            client.state = ClientState::CONNECTED;
        }
    }

    void joinRoom(Client &client)
    {
        // if the room exists
        try
        {
            auto &selectedRoom = room_manager_.findRoomByIndex(client.room_index);
            client.state = ClientState::CHATTING;
            const std::string message = fmt::format("You have joined the room {}", client.room_index);
            sendMessageToClient(client.socket, message);
            selectedRoom.addClient(client);
        }
        // if the room dose not exist
        catch (std::exception &e)
        {
            const std::string message = fmt::format("{}\n", e.what());
            client.state = ClientState::ROOM_LIST_SENT;
            sendMessageToClient(client.socket, message);
        }
    }

    void startChatting(Client &client)
    {
        // TODO: 예외처리 추가 필요?
        auto &selectedRoom = room_manager_.findRoomByIndex(client.room_index);
        const std::string enterMessage = client.username + " has entered the chat.";
        selectedRoom.broadcastMessage(enterMessage, client, true);

        while (server_state_ == ServerState::RUNNING && client.state == ClientState::CHATTING)
        {
            // receive message from the client
            std::array<char, BUFFER_SIZE> buffer = {0};
            auto bytes_received = recv(client.socket, buffer.data(), BUFFER_SIZE, 0);
            const std::string message = std::string(buffer.data(), bytes_received);

            // if the client is disconnected
            if (bytes_received <= 0 || message == "/quit" || message == "/q")
            {
                client.state = ClientState::LEAVING;
                break;
            }
            else if (message == "/back" || message == "/b")
            {
                const std::string message = fmt::format("{} has left the chat.", client.username);
                selectedRoom.broadcastMessage(message, client, true);
                selectedRoom.removeClient(client);
                client.state = ClientState::CONNECTED;
                break;
            }
            else if (message == "/participants" || message == "/p")
            {
                std::string client_lists;
                auto clients = selectedRoom.getClients();
                for (auto client : clients)
                {
                    client_lists += fmt::format(" - {}\n", client.username);
                }
                sendMessageToClient(client.socket, client_lists);
            }
            else
            {
                // broadcast the messages to the clients in the room
                selectedRoom.broadcastMessage(message, client, false);
            }
        }
    }

    void leaveRoom(Client &client)
    {
        auto &selectedRoom = room_manager_.findRoomByIndex(client.room_index);
        const std::string disconnectMessage = client.username + " has left the chat.";
        selectedRoom.broadcastMessage(disconnectMessage, client, true);
        selectedRoom.removeClient(client);
        client.state = ClientState::DEFAULT;
    }

    static bool sendMessageToClient(int socket, const std::string &message)
    {
        auto sendBytes = send(socket, message.c_str(), message.size(), 0);
        return (sendBytes > 0);
    }

    // static std::string recvMessageFromClient(int socket)
    // {
    //     std::string message;
    //     std::array<char, 1024> buffer = {0};

    //     auto bytes_recv = recv(socket, buffer.data(), 1024, 0);
    //     if (bytes_recv < 0)
    //     {
    //         message = "/quit";
    //     }
    //     else
    //     {
    //         message = std::string(buffer.data(), bytes_recv);
    //     }
    //     return message;
    // }

    int acceptClient() const
    {
        const int client_socket = accept(server_socket_, nullptr, nullptr);
        if (client_socket < 0)
        {
            handleError("Failed to accept client connection");
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
    // std::vector<Client> clients_;
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