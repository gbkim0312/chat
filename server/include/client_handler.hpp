#pragma once

#include "client.hpp"
#include "chat_room.hpp"
#include <functional>

namespace network
{
    class ClientHandler
    {
    public:
        ClientHandler(ClientTrigger initial_trigger, ChatRoomManager &room_manager, Client &client);
        ClientState onClientTrigger();

    private:
        ClientTrigger trigger_;
        Client &client_;
        ChatRoomManager &room_manager_;
        ClientState sendOptions();
        ClientState recvSelectedOption();
        ClientState joinRoom();
        ClientState startChatting();
        ClientState createNewRoom();
        ClientState removeRoom();
        ClientState leaveRoom();
        ClientState disconnectClient();
        ClientState showParticipants();
    };
}
