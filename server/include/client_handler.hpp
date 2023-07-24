#pragma once

#include "client.hpp"
#include "chat_room.hpp"
#include <functional>

class ClientHandler
{
public:
    ClientHandler(ClientTrigger initial_trigger);
    ClientState onClientTrigger(Client &client, ChatRoomManager &room_manager);
    void setTrigger(ClientTrigger trigger);

private:
    ClientTrigger trigger_;
    ClientState sendOption(Client &client, ChatRoomManager &room_manager);
    ClientState recvOption(Client &client, ChatRoomManager &room_manager);
    ClientState joinRoom(Client &client, ChatRoomManager &room_manager);
    ClientState startChatting(Client &client, ChatRoomManager &room_manager);
    ClientState createNewRoom(Client &client, ChatRoomManager &room_manager);
    ClientState removeRoom(Client &client, ChatRoomManager &room_manager);
    ClientState leaveRoom(Client &client, ChatRoomManager &room_manager);
    ClientState disconnectClient(Client &client, ChatRoomManager &room_manager);
};
