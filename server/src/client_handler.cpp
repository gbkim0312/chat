#include "client_handler.hpp"
#include <sys/socket.h>
#include <functional>
#include <map>
#include <string>
#include <fmt/core.h>
#include <stdexcept>
#include <array>
#include "client.hpp"
#include "chat_room_manager.hpp"

namespace
{
    constexpr int BUFFER_SIZE = 1024;
}

ClientState ClientHandler::onClientTrigger(Client &client, ChatRoomManager &room_manager, ClientTrigger trigger) // NOLINT
{
    // Define state transition map
    static std::map<ClientState, std::map<ClientTrigger, std::function<ClientState(Client & client, ChatRoomManager & room_manager)>>> stateTransitionMap = {
        {ClientState::CONNECTED, {{ClientTrigger::SEND_ROOMS, [](Client &client, ChatRoomManager &room_manager)
                                   {
                                       // Send room list to client
                                       auto rooms = room_manager.getRooms();

                                       // build string
                                       std::string room_lists_string = "Choose Room from the following List\n(/create to create new room)\n\n";
                                       for (const auto &room : rooms)
                                       {
                                           room_lists_string += fmt::format("{} | {}\n", room.getIndex(), room.getName());
                                       }

                                       // send room lists
                                       sendMessageToClient(client.socket, room_lists_string);

                                       return ClientState::ROOM_LIST_SENT;
                                   }}}},
        {ClientState::ROOM_LIST_SENT, {{ClientTrigger::RECV_ROOMS, [](Client &client, ChatRoomManager &room_manager) // NOLINT
                                        {
                                            // Receive selected room from client
                                            // Logic extracted from recvSelectedRoom() method in ChatServerImpl
                                            const std::string message = recvMessageFromClient(client.socket);

                                            try
                                            {
                                                client.room_index = std::stoi(message);
                                            }
                                            catch (std::invalid_argument &e)
                                            {
                                                sendMessageToClient(client.socket, "Wrong input. Choose again: ");
                                            }

                                            return ClientState::ROOM_SELECTED;
                                        }},
                                       {ClientTrigger::CREATE_ROOM, [](Client &client, ChatRoomManager &room_manager)
                                        {
                                            // Create a new room
                                            const std::string message = recvMessageFromClient(client.socket);
                                            room_manager.createRoom(message, static_cast<int>(room_manager.getRooms().size()), client);
                                            sendMessageToClient(client.socket, "Room created.\n");

                                            return ClientState::CREATING_ROOM;
                                        }},
                                       {ClientTrigger::REMOVE_ROOM, [](Client &client, ChatRoomManager &room_manager)
                                        {
                                            // Remove a room
                                            const std::string message = recvMessageFromClient(client.socket);
                                            const int index = std::stoi(message);
                                            room_manager.removeRoom(index);
                                            sendMessageToClient(client.socket, "Room removed.\n");

                                            return ClientState::REMOVING_ROOM;
                                        }}}},
    };

    // Find the appropriate function in the state transition map based on the client's state and trigger
    auto it = stateTransitionMap.find(client.state);
    if (it != stateTransitionMap.end())
    {
        auto &triggerMap = it->second;
        auto funcIt = triggerMap.find(trigger);
        if (funcIt != triggerMap.end())
        {
            // Call the corresponding function and update the client's state
            return funcIt->second(client, room_manager);
        }
    }

    // If no appropriate state transition is found, return DEFAULT state
    return ClientState::DEFAULT;
}

std::string ClientHandler::recvMessageFromClient(int client_socket)
{
    std::array<char, BUFFER_SIZE> buffer = {0};
    auto bytes_received = recv(client_socket, buffer.data(), BUFFER_SIZE, 0);
    const std::string message = std::string(buffer.data(), bytes_received);

    return message;
}
bool ClientHandler::sendMessageToClient(int socket, const std::string &message)
{
    auto sendBytes = send(socket, message.c_str(), message.size(), 0);
    return (sendBytes > 0);
}