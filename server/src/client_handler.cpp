#include "client_handler.hpp"
#include <functional>
#include <map>
#include <string>
#include <fmt/core.h>
#include <stdexcept>
#include <exception>
#include "client.hpp"
#include "chat_room_manager.hpp"
#include <algorithm>
#include "chat_room.hpp"
#include "network_utility.hpp"
#include <cctype>
#include <vector>

namespace
{
    std::string parseOption(const std::string &option)
    {
        std::string opt = option;
        std::transform(opt.begin(), opt.end(), opt.begin(), [](unsigned char c)
                       { return std::tolower(c); });

        opt.erase(0, opt.find_first_not_of(" \t\r\n"));
        opt.erase(opt.find_last_not_of(" \t\r\n") + 1);
        return opt;
    }

    std::string buildRoomListString(std::vector<ChatRoom> &rooms)
    {
        // sort rooms by index (ascending)
        std::sort(rooms.begin(), rooms.end(), [](const ChatRoom &r1, const ChatRoom &r2)
                  { return r1.getIndex() < r2.getIndex(); });

        // build string
        std::string room_lists_string = "Choose Room from the following List\n(/create to create new room)\n\n";
        for (const auto &room : rooms)
        {
            room_lists_string += fmt::format("{} | {}\n", room.getIndex(), room.getName());
        }
        return room_lists_string;
    }

    // owner의 socket과 client의 socket, owner의 username과 client의 username이 같은 경우 true
    bool isOwner(const Client &owner, const Client &client)
    {
        return (owner.socket == client.socket && owner.username == client.username);
    }
}

ClientHandler::ClientHandler(ClientTrigger initial_trigger) : trigger_(initial_trigger){};

ClientState ClientHandler::onClientTrigger(Client &client, ChatRoomManager &room_manager)
{
    // Define state transition map
    std::map<ClientState, std::map<ClientTrigger, std::function<ClientState(Client & client, ChatRoomManager & room_manager)>>> state_transition_map = {
        {ClientState::CONNECTED, {{ClientTrigger::SEND_ROOMS, [this](Client &client, ChatRoomManager &room_manager)
                                   {
                                       return sendOption(client, room_manager);
                                   }}}},
        {ClientState::OPTION_SENT, {{ClientTrigger::RECV_OPTION, [this](Client &client, ChatRoomManager &room_manager) // NOLINT
                                     {
                                         return recvOption(client, room_manager);
                                     }}}},
        {ClientState::OTPION_SELECTED, {
                                           {ClientTrigger::JOIN_ROOM, [this](Client &client, ChatRoomManager &room_manager)
                                            {
                                                return joinRoom(client, room_manager);
                                            }},
                                           {ClientTrigger::CREATE_ROOM, [this](Client &client, ChatRoomManager &room_manager)
                                            {
                                                return createNewRoom(client, room_manager);
                                            }},
                                           {ClientTrigger::REMOVE_ROOM, [this](Client &client, ChatRoomManager &room_manager)
                                            {
                                                return removeRoom(client, room_manager);
                                            }},
                                       }},
        {ClientState::CHATTING, {
                                    {ClientTrigger::START_CHAT, [this](Client &client, ChatRoomManager &room_manager)
                                     {
                                         return startChatting(client, room_manager);
                                     }},
                                    {ClientTrigger::LEAVE, [this](Client &client, ChatRoomManager &room_manager)
                                     {
                                         return leaveRoom(client, room_manager);
                                     }},
                                }},
        {ClientState::DISCONNECTED, {{
                                        {ClientTrigger::DISCONNECT, [this](Client &client, ChatRoomManager &room_manager)
                                         {
                                             return disconnectClient(client, room_manager);
                                         }},
                                    }}},

    };

    // Find the appropriate function in the state transition map based on the client's state and trigger
    auto it = state_transition_map.find(client.state);
    if (it != state_transition_map.end())
    {
        // fmt::print("State found: {}\n", static_cast<int>(client.state));
        auto &trigger_map = it->second;
        auto function_it = trigger_map.find(trigger_);
        if (function_it != trigger_map.end())
        {
            // Call the corresponding function and update the client's state
            return function_it->second(client, room_manager);
        }
        else
        {
            fmt::println("[ERROR] Cannot find trigger: {} in state: {}", static_cast<int>(trigger_), static_cast<int>(client.state));
        }
    }
    // If no appropriate state transition is found, return DEFAULT state
    return ClientState::DEFAULT;
}

ClientState ClientHandler::sendOption(Client &client, ChatRoomManager &room_manager)
{

    // Send room list to client
    auto rooms = room_manager.getRooms();

    // TODO: Trigger로 처리
    if (rooms.empty())
    {
        network::sendMessageToClient(client.socket, "No rooms. do you want to create new room? (y|n)");
        const std::string opt = parseOption(network::recvMessageFromClient(client.socket));
        if (opt.empty())
        {
            trigger_ = ClientTrigger::DISCONNECT;
            return ClientState::DISCONNECTED;
        }
        else if (opt == "y")
        {
            trigger_ = ClientTrigger::CREATE_ROOM;
            return ClientState::OTPION_SELECTED;
        }
        else if (opt == "n")
        {
            trigger_ = ClientTrigger::SEND_ROOMS;
            return ClientState::CONNECTED;
        }
        else
        {
            network::sendMessageToClient(client.socket, "Wrong Input.\n");
            trigger_ = ClientTrigger::SEND_ROOMS;
            return ClientState::CONNECTED;
        }
    }

    const std::string room_list_string = buildRoomListString(rooms);

    // send room lists
    network::sendMessageToClient(client.socket, room_list_string);
    trigger_ = ClientTrigger::RECV_OPTION;

    return ClientState::OPTION_SENT;
}

ClientState ClientHandler::recvOption(Client &client, ChatRoomManager &room_manager)
{
    auto rooms = room_manager.getRooms();
    const std::string opt = parseOption(network::recvMessageFromClient(client.socket));
    if (opt.empty())
    {
        this->trigger_ = ClientTrigger::DISCONNECT;
        return ClientState::DISCONNECTED;
    }
    else if (opt == "/help" || opt == "/h")
    {
        network::sendMessageToClient(client.socket, "command list: /help /create /reload /remove");
        this->trigger_ = ClientTrigger::RECV_OPTION;
        return ClientState::OPTION_SENT;
    }
    else if (opt == "/create" || opt == "/c")
    {
        this->trigger_ = ClientTrigger::CREATE_ROOM;
        return ClientState::OTPION_SELECTED;
    }
    else if (opt == "/reload" || opt == "/r")
    {
        this->trigger_ = ClientTrigger::SEND_ROOMS;
        return ClientState::CONNECTED;
    }
    else if (opt == "/remove" || opt == "/rm")
    {
        this->trigger_ = ClientTrigger::REMOVE_ROOM;
        return ClientState::OTPION_SELECTED;
    }
    else
    {
        try
        {
            client.room_index = std::stoi(opt);
        }
        catch (std::invalid_argument &e)
        {
            network::sendMessageToClient(client.socket, "Wrong input. Choose again: ");
            this->trigger_ = ClientTrigger::RECV_OPTION;
            return ClientState::OPTION_SENT;
        }
        this->trigger_ = ClientTrigger::JOIN_ROOM;
        return ClientState::OTPION_SELECTED;
    }
}

ClientState ClientHandler::joinRoom(Client &client, ChatRoomManager &room_manager)
{
    try
    {
        auto &selected_room = room_manager.findRoomByIndex(client.room_index);
        const std::string message = fmt::format("You have joined the room \"{}\"\n", selected_room.getName());
        network::sendMessageToClient(client.socket, message);
        selected_room.addClient(client);
    }
    // if the room dose not exist
    catch (std::exception &e)
    {
        const std::string message = fmt::format("{}\n", e.what());
        network::sendMessageToClient(client.socket, message);
        trigger_ = ClientTrigger::RECV_OPTION;
        return ClientState::OPTION_SENT;
    }
    trigger_ = ClientTrigger::START_CHAT;
    return ClientState::CHATTING;
}

ClientState ClientHandler::startChatting(Client &client, ChatRoomManager &room_manager)
{

    auto &selected_room = room_manager.findRoomByIndex(client.room_index);
    const std::string enter_message = fmt::format("{} has joined the room", client.username);
    selected_room.broadcastMessage(enter_message, client, true);

    while (client.state == ClientState::CHATTING)
    {
        // receive message from the client
        const std::string message = parseOption(network::recvMessageFromClient(client.socket));

        if (message.empty() || message == "/quit" || message == "/q")
        {
            trigger_ = ClientTrigger::DISCONNECT;
            return ClientState::DISCONNECTED;
        }
        else if (message == "/back" || message == "/b")
        {
            trigger_ = ClientTrigger::LEAVE;
            return ClientState::CHATTING;
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
    return ClientState::DEFAULT;
}

ClientState ClientHandler::createNewRoom(Client &client, ChatRoomManager &room_manager)
{

    network::sendMessageToClient(client.socket, "Input room name: ");
    const std::string message = network::recvMessageFromClient(client.socket);

    if (message.empty())
    {
        trigger_ = ClientTrigger::DISCONNECT;
        return ClientState::DISCONNECTED;
    }
    else
    {
        room_manager.createRoom(message, static_cast<int>(room_manager.getRooms().size()), client); // rooms의 size의 index 부여
        trigger_ = ClientTrigger::SEND_ROOMS;
        return ClientState::CONNECTED;
    }
}

ClientState ClientHandler::removeRoom(Client &client, ChatRoomManager &room_manager)
{
    network::sendMessageToClient(client.socket, "Input room index to remove ('/back' to go back) ");
    const std::string opt = parseOption(network::recvMessageFromClient(client.socket));

    if (opt.empty())
    {
        trigger_ = ClientTrigger::DISCONNECT;
        return ClientState::DISCONNECTED;
    }
    else if (opt == "/back" || opt == "/b")
    {
        trigger_ = ClientTrigger::SEND_ROOMS;
        return ClientState::CONNECTED;
    }
    else
    {
        try
        {
            const int index = std::stoi(opt);
            auto selected_room = room_manager.findRoomByIndex(index);

            // TODO: client id 구현 후, id로 지우기 (username으로 하면, username이 같은 경우 문제 발생가능, 현재는 Socket으로 구현)
            if (isOwner(selected_room.getOwner(), client))
            {
                selected_room.broadcastMessage("Room Closed", client, true);
                room_manager.removeRoom(index);
                network::sendMessageToClient(client.socket, "Room removed.\n");
                trigger_ = ClientTrigger::SEND_ROOMS;
                return ClientState::CONNECTED;
            }
            else
            {
                network::sendMessageToClient(client.socket, "You have no permission. Choose again\n\n");
                trigger_ = ClientTrigger::REMOVE_ROOM;
                return ClientState::OTPION_SELECTED;
            }
        }
        catch (std::invalid_argument &e)
        {
            network::sendMessageToClient(client.socket, "invalid input. Choose again\n\n");
            trigger_ = ClientTrigger::REMOVE_ROOM;
            return ClientState::OTPION_SELECTED;
        }
        catch (std::runtime_error &e)
        {
            network::sendMessageToClient(client.socket, "Room not found. choose again\n\n");
            trigger_ = ClientTrigger::REMOVE_ROOM;
            return ClientState::OTPION_SELECTED;
        }
    }
}

ClientState ClientHandler::leaveRoom(Client &client, ChatRoomManager &room_manager)
{
    try
    {
        auto &selected_room = room_manager.findRoomByIndex(client.room_index);
        const std::string disconnectMessage = client.username + " has left the chat.";
        selected_room.broadcastMessage(disconnectMessage, client, true);
        selected_room.removeClient(client);
    }
    catch (std::runtime_error &e)
    {
        network::sendMessageToClient(client.socket, "Leaving the room.");
    }
    client.room_index = -1;
    trigger_ = ClientTrigger::SEND_ROOMS;
    return ClientState::CONNECTED;
}

ClientState ClientHandler::disconnectClient(Client &client, ChatRoomManager &room_manager)
{
    if (client.room_index > -1)
    {
        auto selectedRoom = room_manager.findRoomByIndex(client.room_index);
        selectedRoom.removeClient(client);
        trigger_ = ClientTrigger::LEAVE;
        return ClientState::CHATTING;
    }

    return ClientState::DEFAULT;
}