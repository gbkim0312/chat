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
    // 모든 문자를 소문자로 만들고, 앞뒤 개행문자, 스페이스 등 제거
    std::string normalizeOption(const std::string &option)
    {
        std::string opt = option;
        std::transform(opt.begin(), opt.end(), opt.begin(), [](unsigned char c)
                       { return std::tolower(c); });

        network::removeWhitespaces(opt);

        return opt;
    }

    // 채팅방 목록을 위한 스트링 생성
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
    // operator overloading operator==&
    bool isOwner(const network::Client &owner, const network::Client &client)
    {
        return (owner.socket == client.socket && owner.username == client.username);
    }
}

namespace network
{
    ClientHandler::ClientHandler(ClientTrigger initial_trigger, ChatRoomManager &room_manager, Client &client)
        : trigger_(initial_trigger), room_manager_(room_manager), client_(client){};

    ClientState ClientHandler::onClientTrigger()
    {
        // State Transition Map 정의
        // 함수를 객체로 바로 넣어야함, map을 다른 곳에서 선언, lambda function 내 this 금지
        std::map<ClientState, std::map<ClientTrigger, std::function<ClientState()>>> state_transition_map = {
            {ClientState::CONNECTED, {{ClientTrigger::SEND_OPTIONS, [this]()
                                       { return sendOptions(); }}}},
            {ClientState::OPTION_SENT, {{ClientTrigger::RECV_OPTION, [this]()
                                         {
                                             return recvSelectedOption();
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
                                               {ClientTrigger::SHOW_PARITCIPANTS, [this]()
                                                {
                                                    return showParticipants();
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

        // 클라이언트의 상태와 트리거에 따른 적절한 함수 찾기
        auto it = state_transition_map.find(client_.state);
        if (it != state_transition_map.end())
        {
            // fmt::print("State found: {}\n", static_cast<int>(client.state));
            auto &trigger_map = it->second;
            auto function_it = trigger_map.find(trigger_);
            // 함수를 찾으면
            if (function_it != trigger_map.end())
            {
                return function_it->second();
            }
            else
            {
                fmt::println("[ERROR] Cannot find trigger: {} in state: {}", static_cast<int>(trigger_), static_cast<int>(client_.state));
            }
        }
        // 적절한 상태와 트리거를 찾지 못한 경우
        // 못찾았을 때를 if로 넣고, (초기 조건 확인하는 경우), 최대한 depth를 얕게 가져가야함
        return ClientState::DEFAULT;
    }

    // 옵션을 보냄(채팅방 목록)
    ClientState ClientHandler::sendOptions()
    {
        auto rooms = room_manager_.getRooms();
        // 채팅방이 존재하지 않는 경우
        if (rooms.empty())
        {
            sendMessageToClient(client_.socket, "No rooms. do you want to create new room? (y|n)");
            const std::string opt = normalizeOption(recvMessageFromClient(client_.socket));
            // 함수로 작성 가능하면 하도록 (depth를 낮출 수 있다.)
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
                trigger_ = ClientTrigger::SEND_OPTIONS;
                return ClientState::CONNECTED;
            }
            else
            {
                sendMessageToClient(client_.socket, "Wrong Input.\n");
                trigger_ = ClientTrigger::SEND_OPTIONS;
                return ClientState::CONNECTED;
            }
        }

        // 채팅방 목록 string 생성 후 전송
        sendMessageToClient(client_.socket, buildRoomListString(rooms));
        trigger_ = ClientTrigger::RECV_OPTION;

        return ClientState::OPTION_SENT;
    }

    // 클라이언트가 선택한 옵션 받기
    ClientState ClientHandler::recvSelectedOption()
    {
        auto rooms = room_manager_.getRooms();
        const std::string opt = normalizeOption(recvMessageFromClient(client_.socket));

        // 3개 이상일 경우 Switch 사용. Mapping
        // enum class로 바꿔서
        if (opt.empty())
        {
            trigger_ = ClientTrigger::DISCONNECT;
            return ClientState::DISCONNECTED;
        }
        else if (opt == "/help" || opt == "/h")
        {
            sendMessageToClient(client_.socket, "command list: /help (/h) /create (/c) /reload (/r) /remove (/rm) /participants (/p)");
            trigger_ = ClientTrigger::RECV_OPTION;
            return ClientState::OPTION_SENT;
        }
        else if (opt == "/create" || opt == "/c")
        {
            trigger_ = ClientTrigger::CREATE_ROOM;
            return ClientState::OTPION_SELECTED;
        }
        else if (opt == "/reload" || opt == "/r")
        {
            trigger_ = ClientTrigger::SEND_OPTIONS;
            return ClientState::CONNECTED;
        }
        else if (opt == "/remove" || opt == "/rm")
        {
            trigger_ = ClientTrigger::REMOVE_ROOM;
            return ClientState::OTPION_SELECTED;
        }
        // TODO: 채팅 참가자 목록 보기 추가
        else if (opt == "/participants" || opt == "/p")
        {
            trigger_ = ClientTrigger::SHOW_PARITCIPANTS;
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
                trigger_ = ClientTrigger::RECV_OPTION;
                return ClientState::OPTION_SENT;
            }
            trigger_ = ClientTrigger::JOIN_ROOM;
            return ClientState::OTPION_SELECTED;
        }
    }

    // 채팅방에 클라이언트를 추가
    ClientState ClientHandler::joinRoom()
    {
        // 클라리언트가 선택한 인덱스로 채팅방을 찾고, 클라이언트 추가
        try
        {
            auto &selected_room = room_manager_.findRoomByIndex(client_.room_index);
            const std::string message = fmt::format("You have joined the room \"{}\"\n", selected_room.getName());
            sendMessageToClient(client_.socket, message);
            selected_room.addClient(client_);
        }
        // 존재하지 않는 방을 선택한 경우
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

    // 채팅 시작
    ClientState ClientHandler::startChatting()
    {
        auto &selected_room = room_manager_.findRoomByIndex(client_.room_index);
        const std::string welcome_message = fmt::format("{} has joined the room", client_.username);
        selected_room.broadcastMessage(welcome_message, client_, MessageType::NOTICE);

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
                // message는 normalize하면 안됨;
                selected_room.broadcastMessage(message, client_);
            }
        }
        return ClientState::DEFAULT;
        // std::message
    }

    // 방 생성
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
            trigger_ = ClientTrigger::SEND_OPTIONS;
            return ClientState::CONNECTED;
        }
    }

    // 방 삭제
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
            trigger_ = ClientTrigger::SEND_OPTIONS;
            return ClientState::CONNECTED;
        }
        else
        {
            try
            {
                const int index = std::stoi(opt);
                auto selected_room = room_manager_.findRoomByIndex(index);

                // 현재 방을 삭제하려고 하는 클라이언트가 방장인지 확인
                if (isOwner(selected_room.getOwner(), client_))
                {
                    selected_room.broadcastMessage("LEAVE", client_, MessageType::COMMAND);
                    room_manager_.removeRoom(index);
                    sendMessageToClient(client_.socket, "Room removed.\n\n");
                    trigger_ = ClientTrigger::SEND_OPTIONS;
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

    // 방 나가기
    ClientState ClientHandler::leaveRoom()
    {
        try
        {
            auto &selected_room = room_manager_.findRoomByIndex(client_.room_index);
            const std::string message = fmt::format("{} has left the chat.", client_.username);
            selected_room.broadcastMessage(message, client_, MessageType::NOTICE);
            selected_room.removeClient(client_);
        }
        catch (std::runtime_error &e)
        {
        }

        client_.room_index = -1;
        trigger_ = ClientTrigger::SEND_OPTIONS;
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

    ClientState ClientHandler::showParticipants()
    {
        sendMessageToClient(client_.socket, "Choose room index to see a list of participants: ");
        const std::string opt = normalizeOption(recvMessageFromClient(client_.socket));
        if (opt.empty())
        {
            trigger_ = ClientTrigger::DISCONNECT;
            return ClientState::DISCONNECTED;
        }
        try
        {
            auto selected_room = room_manager_.findRoomByIndex(std::stoi(opt));
            auto participants = selected_room.getClients();

            std::string message;
            if (participants.empty())
            {
                message = fmt::format("\nThere are no clients in the room [{}]\n", selected_room.getName());
            }
            else
            {
                message = fmt::format("\nParticipants list of the room [{}]\n", selected_room.getName());
                for (const auto &p : participants)
                {
                    message += fmt::format("* {}\n", p.username);
                }
            }
            sendMessageToClient(client_.socket, message);
            trigger_ = ClientTrigger::SEND_OPTIONS;
            return ClientState::CONNECTED;
        }
        catch (const std::runtime_error &e)
        {
            sendMessageToClient(client_.socket, "No room found. Choose again");
            trigger_ = ClientTrigger::SHOW_PARITCIPANTS;
            return ClientState::OTPION_SELECTED;
        }
        catch (const std::invalid_argument &e)
        {
            sendMessageToClient(client_.socket, "Invalid input. Choose again");
            trigger_ = ClientTrigger::SHOW_PARITCIPANTS;
            return ClientState::OTPION_SELECTED;
        }
        trigger_ = ClientTrigger::SEND_OPTIONS;
        return ClientState::CONNECTED;
    }
}