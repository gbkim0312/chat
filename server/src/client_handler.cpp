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
    std::string normalizeOption(const std::string &option)
    {
        std::string opt = option;
        std::transform(opt.begin(), opt.end(), opt.begin(), [](unsigned char c)
                       { return std::tolower(c); });

        network::removeWhitespaces(opt);

        return opt;
    }

    std::string buildRoomListString(std::vector<network::ChatRoom> &rooms)
    {
        // sort rooms by index (ascending)
        std::sort(rooms.begin(), rooms.end(), [](const network::ChatRoom &r1, const network::ChatRoom &r2)
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
    bool isOwner(const network::Client &owner, const network::Client &client)
    {
        return (owner.socket == client.socket && owner.username == client.username);
    }
}
namespace network
{

    ClientHandler::ClientHandler(ClientTrigger initial_trigger, ChatRoomManager &room_manager, Client &client) : trigger_(initial_trigger), room_manager_(room_manager), client_(client){};

    ClientState ClientHandler::onClientTrigger()
    {
        // Define state transition map
        std::map<ClientState, std::map<ClientTrigger, std::function<ClientState()>>> state_transition_map = {
            {ClientState::CONNECTED, {{ClientTrigger::SEND_ROOMS, [this]()
                                       { return sendOption(); }}}},
            {ClientState::OPTION_SENT, {{ClientTrigger::RECV_OPTION, [this]()
                                         {
                                             return recvOption();
                                         }}}},
            {ClientState::OTPION_SELECTED, {
                                               {ClientTrigger::JOIN_ROOM, [this]()
                                                {
                                                    return joinRoom();
                                                }},
                                               {ClientTrigger::CREATE_ROOM, [this]()
                                                {
                                                    return createNewRoom();
                                                }},
                                               {ClientTrigger::REMOVE_ROOM, [this]()
                                                {
                                                    return removeRoom();
                                                }},
                                           }},
            {ClientState::CHATTING, {
                                        {ClientTrigger::START_CHAT, [this]()
                                         {
                                             return startChatting();
                                         }},
                                        {ClientTrigger::LEAVE, [this]()
                                         {
                                             return leaveRoom();
                                         }},
                                    }},
            {ClientState::DISCONNECTED, {{
                                            {ClientTrigger::DISCONNECT, [this]()
                                             {
                                                 return disconnectClient();
                                             }},
                                        }}},

        };

        // Find the appropriate function in the state transition map based on the client's state and trigger
        auto it = state_transition_map.find(client_.state);
        if (it != state_transition_map.end())
        {
            // fmt::print("State found: {}\n", static_cast<int>(client.state));
            auto &trigger_map = it->second;
            auto function_it = trigger_map.find(trigger_);
            if (function_it != trigger_map.end())
            {
                // Call the corresponding function and update the client's state
                return function_it->second();
            }
            else
            {
                fmt::println("[ERROR] Cannot find trigger: {} in state: {}", static_cast<int>(trigger_), static_cast<int>(client_.state));
            }
        }
        // If no appropriate state transition is found, return DEFAULT state
        return ClientState::DEFAULT;
    }

    ClientState ClientHandler::sendOption()
    {

        // Send room list to client
        auto rooms = room_manager_.getRooms();

        // TODO: Trigger로 처리
        if (rooms.empty())
        {
            sendMessageToClient(client_.socket, "No rooms. do you want to create new room? (y|n)");
            const std::string opt = normalizeOption(recvMessageFromClient(client_.socket));
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
                sendMessageToClient(client_.socket, "Wrong Input.\n");
                trigger_ = ClientTrigger::SEND_ROOMS;
                return ClientState::CONNECTED;
            }
        }

        const std::string room_list_string = buildRoomListString(rooms);

        // send room lists
        sendMessageToClient(client_.socket, room_list_string);
        trigger_ = ClientTrigger::RECV_OPTION;

        return ClientState::OPTION_SENT;
    }

    ClientState ClientHandler::recvOption()
    {
        auto rooms = room_manager_.getRooms();
        const std::string opt = normalizeOption(recvMessageFromClient(client_.socket));
        if (opt.empty())
        {
            this->trigger_ = ClientTrigger::DISCONNECT;
            return ClientState::DISCONNECTED;
        }
        else if (opt == "/help" || opt == "/h")
        {
            sendMessageToClient(client_.socket, "command list: /help /create /reload /remove");
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
                client_.room_index = std::stoi(opt);
            }
            catch (std::invalid_argument &e)
            {
                sendMessageToClient(client_.socket, "Wrong input. Choose again: ");
                this->trigger_ = ClientTrigger::RECV_OPTION;
                return ClientState::OPTION_SENT;
            }
            this->trigger_ = ClientTrigger::JOIN_ROOM;
            return ClientState::OTPION_SELECTED;
        }
    }

    ClientState ClientHandler::joinRoom()
    {
        try
        {
            auto &selected_room = room_manager_.findRoomByIndex(client_.room_index);
            const std::string message = fmt::format("You have joined the room \"{}\"\n", selected_room.getName());
            sendMessageToClient(client_.socket, message);
            selected_room.addClient(client_);
        }
        // if the room dose not exist
        catch (std::exception &e)
        {
            const std::string message = fmt::format("{}\n", e.what());
            sendMessageToClient(client_.socket, message);
            trigger_ = ClientTrigger::RECV_OPTION;
            return ClientState::OPTION_SENT;
        }
        trigger_ = ClientTrigger::START_CHAT;
        return ClientState::CHATTING;
    }

    ClientState ClientHandler::startChatting()
    {

        auto &selected_room = room_manager_.findRoomByIndex(client_.room_index);
        const std::string enter_message = fmt::format("{} has joined the room", client_.username);
        selected_room.broadcastMessage(enter_message, client_, MessageType::NOTICE);

        while (client_.state == ClientState::CHATTING)
        {
            // receive message from the client
            const std::string message = normalizeOption(recvMessageFromClient(client_.socket));

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
                selected_room.sendParticipantsList(client_.socket);
            }
            else
            {
                selected_room.broadcastMessage(message, client_);
            }
        }
        return ClientState::DEFAULT;
    }

    ClientState ClientHandler::createNewRoom()
    {

        sendMessageToClient(client_.socket, "Input room name: ");
        const std::string message = recvMessageFromClient(client_.socket);

        if (message.empty())
        {
            trigger_ = ClientTrigger::DISCONNECT;
            return ClientState::DISCONNECTED;
        }
        else
        {
            room_manager_.createRoom(message, static_cast<int>(room_manager_.getRooms().size()), client_); // rooms의 size의 index 부여
            trigger_ = ClientTrigger::SEND_ROOMS;
            return ClientState::CONNECTED;
        }
    }

    ClientState ClientHandler::removeRoom()
    {
        sendMessageToClient(client_.socket, "Input room index to remove ('/back' to go back) ");
        const std::string opt = normalizeOption(recvMessageFromClient(client_.socket));

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
                auto selected_room = room_manager_.findRoomByIndex(index);

                // TODO: client id 구현 후, id로 지우기 (username으로 하면, username이 같은 경우 문제 발생가능, 현재는 Socket으로 구현)
                if (isOwner(selected_room.getOwner(), client_))
                {
                    // selected_room.broadcastMessage("Room Closed", client, MessageType::NOTICE);
                    selected_room.broadcastMessage("LEAVE", client_, MessageType::COMMAND);
                    room_manager_.removeRoom(index);
                    sendMessageToClient(client_.socket, "Room removed.\n\n");
                    trigger_ = ClientTrigger::SEND_ROOMS;
                    return ClientState::CONNECTED;
                }
                else
                {
                    sendMessageToClient(client_.socket, "You have no permission. Choose again\n\n");
                    trigger_ = ClientTrigger::REMOVE_ROOM;
                    return ClientState::OTPION_SELECTED;
                }
            }
            catch (std::invalid_argument &e)
            {
                sendMessageToClient(client_.socket, "invalid input. Choose again\n\n");
                trigger_ = ClientTrigger::REMOVE_ROOM;
                return ClientState::OTPION_SELECTED;
            }
            catch (std::runtime_error &e)
            {
                sendMessageToClient(client_.socket, "Room not found. choose again\n\n");
                trigger_ = ClientTrigger::REMOVE_ROOM;
                return ClientState::OTPION_SELECTED;
            }
        }
    }

    ClientState ClientHandler::leaveRoom()
    {
        try
        {
            auto &selected_room = room_manager_.findRoomByIndex(client_.room_index);
            const std::string disconnectMessage = client_.username + " has left the chat.";
            selected_room.broadcastMessage(disconnectMessage, client_, MessageType::NOTICE);
            selected_room.removeClient(client_);
        }
        catch (std::runtime_error &e)
        {
        }

        client_.room_index = -1;
        trigger_ = ClientTrigger::SEND_ROOMS;
        return ClientState::CONNECTED;
    }

    ClientState ClientHandler::disconnectClient()
    {
        if (client_.room_index > -1)
        {
            auto selectedRoom = room_manager_.findRoomByIndex(client_.room_index);
            selectedRoom.removeClient(client_);
            trigger_ = ClientTrigger::LEAVE;
            return ClientState::CHATTING;
        }
        return ClientState::DEFAULT;
    }
}