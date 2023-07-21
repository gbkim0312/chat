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
#include <client_handler.hpp>

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

        room_manager_.createDefaultRooms(3);
    }

    // // Handle each client independently according to their state
    // void handleClient(Client client)
    // {
    //     // bool stop = false;
    //     while (server_state_ == ServerState::RUNNING)
    //     {
    //         const ClientState client_state = client.state;
    //         if (client_state == ClientState::DEFAULT)
    //         {
    //             break;
    //         }

    //         switch (client_state)
    //         {
    //         case ClientState::CONNECTED:
    //             sendRoomLists(client);
    //             break;
    //         case ClientState::ROOM_LIST_SENT:
    //             recvSelectedRoom(client);
    //             break;
    //         case ClientState::ROOM_SELECTED:
    //             joinRoom(client);
    //             break;
    //         case ClientState::CREATING_ROOM:
    //             createNewRoom(client);
    //             break;
    //         case ClientState::REMOVING_ROOM:
    //             removeRoom(client);
    //             break;
    //         case ClientState::CHATTING:
    //             startChatting(client);
    //             break;
    //         case ClientState::LEAVING:
    //             leaveRoom(client);
    //             break;
    //         case ClientState::DISCONNECTED:
    //             disconnectClient(client);
    //             break;
    //         // TODO: DEFAULT 상태일 때(연결이 끊어졌을 때) 동작 추가
    //         default:
    //             break;
    //         }
    //     }
    // }
    void handleClient(Client client)
    {
        ClientHandler handler;
        ClientTrigger trigger = ClientTrigger::SEND_ROOMS; // 초기 트리거 설정

        while (server_state_ == ServerState::RUNNING)
        {
            const ClientState newState = handler.onClientTrigger(client, room_manager_, trigger);

            client.state = newState;

            trigger = ClientTrigger::SEND_ROOMS;
        }
    }

    bool sendRoomLists(Client &client)
    {
        auto rooms = room_manager_.getRooms();

        if (rooms.empty())
        {
            sendMessageToClient(client.socket, "No rooms. do you want to create new room? (y|n)");
            const std::string message = recvMessageFromClient(client.socket);
            if (message.empty())
            {
                client.state = ClientState::DISCONNECTED;
            }
            else if (message == "y" || message == "Y")
            {
                client.state = ClientState::CREATING_ROOM;
            }
            else if (message == "n" || message == "N")
            {
                client.state = ClientState::CONNECTED;
            }
            else
            {
                sendMessageToClient(client.socket, "Wrong Input.\n");
                client.state = ClientState::CONNECTED;
            }
            return false;
        };

        // sort rooms by index (ascending)
        std::sort(rooms.begin(), rooms.end(), [](const ChatRoom &r1, const ChatRoom &r2)
                  { return r1.getIndex() < r2.getIndex(); });

        // build string
        std::string room_lists_string = "Choose Room from the following List\n(/create to create new room)\n\n";
        for (const auto &room : rooms)
        {
            room_lists_string += fmt::format("{} | {}\n", room.getIndex(), room.getName());
        }

        // send room lists
        if (sendMessageToClient(client.socket, room_lists_string))
        {
            client.state = ClientState::ROOM_LIST_SENT;
            return true;
        }
        else
        {
            handleError("Fail to send Room Lists");
            client.state = ClientState::CONNECTED;
            return false;
        }
    }

    static void recvSelectedRoom(Client &client)
    {
        const std::string message = recvMessageFromClient(client.socket);
        client.state = ClientState::ROOM_SELECTED;

        if (message.empty())
        {
            client.state = ClientState::DISCONNECTED;
        }
        else if (message == "/help" || message == "/h")
        {
            sendMessageToClient(client.socket, "command list: /help /create /reload /remove");
            client.state = ClientState::ROOM_LIST_SENT;
        }
        else if (message == "/create")
        {
            client.state = ClientState::CREATING_ROOM;
        }
        else if (message == "/reload")
        {
            client.state = ClientState::CONNECTED;
        }
        else if (message == "/remove" || message == "/rm")
        {
            client.state = ClientState::REMOVING_ROOM;
        }
        else
        {
            try
            {
                client.room_index = std::stoi(message);
            }
            catch (std::invalid_argument &e)
            {
                sendMessageToClient(client.socket, "Wrong input. Choose again: ");
                client.state = ClientState::ROOM_LIST_SENT;
            }
        }
    }

    void createNewRoom(Client &client)
    {
        sendMessageToClient(client.socket, "Input room name: ");
        const std::string message = recvMessageFromClient(client.socket);

        if (message.empty())
        {
            client.state = ClientState::DISCONNECTED;
        }
        else
        {
            room_manager_.createRoom(message, static_cast<int>(room_manager_.getRooms().size()), client); // rooms의 size의 index 부여
            client.state = ClientState::CONNECTED;
        }
    }

    void removeRoom(Client &client)
    {
        sendMessageToClient(client.socket, "Input room index to remove ('/back' to go back) ");
        std::string message = recvMessageFromClient(client.socket);

        if (message.empty())
        {
            client.state = ClientState::DISCONNECTED;
        }
        else if (message == "/back" || message == "/b")
        {
            client.state = ClientState::CONNECTED;
        }
        else
        {
            try
            {
                const int index = std::stoi(message);
                auto selected_room = room_manager_.findRoomByIndex(index);

                // TODO: client id 구현 후, id로 지우기 (username으로 하면, username이 같은 경우 문제 발생가능)
                // Socket으로 대체해도 될까? -> 문제가 있을듯
                // 방을 생성한 사람만 지울 수 있도록
                if (selected_room.getOwner().socket == client.socket)
                {
                    selected_room.broadcastMessage("Room Closed", admin_, true);
                    room_manager_.removeRoom(index);
                    message = "Room removed.\n";
                    client.state = ClientState::CONNECTED;
                }
                else
                {
                    message = "You have no permission. Choose again\n\n";
                    client.state = ClientState::REMOVING_ROOM;
                }
                sendMessageToClient(client.socket, message);
            }
            catch (std::invalid_argument &e)
            {
                sendMessageToClient(client.socket, "invalid input. Choose again\n\n");
                client.state = ClientState::REMOVING_ROOM;
            }
            catch (std::runtime_error &e)
            {
                sendMessageToClient(client.socket, "Room not found. choose again\n\n");
                client.state = ClientState::REMOVING_ROOM;
            }
        }
    }

    void joinRoom(Client &client)
    {
        // if the room exists
        try
        {
            auto &selected_room = room_manager_.findRoomByIndex(client.room_index);
            client.state = ClientState::CHATTING;
            const std::string message = fmt::format("You have joined the room \"{}\"\n", selected_room.getName());
            sendMessageToClient(client.socket, message);
            selected_room.addClient(client);
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
        auto &selected_room = room_manager_.findRoomByIndex(client.room_index);
        const std::string enterMessage = fmt::format("{} has joined the room", client.username);
        selected_room.broadcastMessage(enterMessage, client, true);

        while (server_state_ == ServerState::RUNNING && client.state == ClientState::CHATTING)
        {
            // receive message from the client
            const std::string message = recvMessageFromClient(client.socket);

            if (message.empty() || message == "/quit" || message == "/q")
            {
                client.state = ClientState::DISCONNECTED;
                break;
            }
            else if (message == "/back" || message == "/b")
            {
                client.state = ClientState::LEAVING;
                break;
            }
            else if (message == "/participants" || message == "/p")
            {
                selected_room.sendParticipantsList(client.socket);
            }
            else
            {
                selected_room.broadcastMessage(message, client);
            }
        }
    }

    void leaveRoom(Client &client)
    {
        try
        {
            auto &selected_room = room_manager_.findRoomByIndex(client.room_index);
            const std::string disconnectMessage = client.username + " has left the chat.";
            selected_room.broadcastMessage(disconnectMessage, client, true);
            selected_room.removeClient(client);
        }
        // 방이 삭제되어서 leaveRoom 함수 호출하는 경우
        catch (std::runtime_error &e)
        {
        }
        client.room_index = -1;
        client.state = ClientState::CONNECTED;
    }

    void disconnectClient(Client &client)
    {
        if (client.room_index > -1)
        {
            leaveRoom(client);
        }
        close(client.socket);
        client_sockets_.erase(client.socket);
        client.state = ClientState::DEFAULT;
        fmt::print("Client {} is disconnected\n", client.username);
    }

    static void handleError(const std::string &error_message)
    {
        fmt::print("runtime error: {}\n", error_message);
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
        std::string username = recvMessageFromClient(client_socket);
        if (username.empty())
        {
            handleError("Failed to receive message");
        }
        username.erase(0, username.find_first_not_of(" \t\r\n"));
        username.erase(username.find_last_not_of(" \t\r\n") + 1);

        return username;
    }

    static Client createClient(int client_socket, const std::string &username)
    {
        Client client;
        // client.id = ++client_id_;
        client.socket = client_socket;
        client.state = ClientState::CONNECTED;
        client.username = username;
        return client;
    }

    static bool sendMessageToClient(int socket, const std::string &message)
    {
        auto sendBytes = send(socket, message.c_str(), message.size(), 0);
        return (sendBytes > 0);
    }

    int acceptClient() const
    {
        const int client_socket = accept(server_socket_, nullptr, nullptr);
        if (client_socket < 0)
        {
            handleError("Failed to accept client connection");
        }
        return client_socket;
    }

    static std::string recvMessageFromClient(int client_socket)
    {
        std::array<char, BUFFER_SIZE> buffer = {0};
        auto bytes_received = recv(client_socket, buffer.data(), BUFFER_SIZE, 0);
        const std::string message = std::string(buffer.data(), bytes_received);

        return message;
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