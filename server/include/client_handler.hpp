#pragma once

#include "client.hpp"
#include "chat_room.hpp"
#include <functional>

class ClientHandler
{
public:
    ClientState onClientTrigger(Client &client, ChatRoomManager &room_manager, ClientTrigger trigger);

private:
    static std::string recvMessageFromClient(int client_socket);
    static bool sendMessageToClient(int socket, const std::string &message);
};
