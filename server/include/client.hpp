#pragma once

#include <string>

enum class ClientState
{
    CONNECTED,
    ROOM_LIST_SENT,
    CREATING_ROOM,
    REMOVING_ROOM,
    ROOM_SELECTED,
    CHATTING,
    LEAVING,
    DEFAULT
};

struct Client
{
    // int id = 0; //socket이 id 대신함
    int socket = -1;
    ClientState state = ClientState::DEFAULT;
    std::string username = "";
    int room_index = 1;
};
