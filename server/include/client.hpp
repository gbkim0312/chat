#pragma once

#include <string>

enum class ClientState
{
    CONNECTED,
    ROOM_LIST_SENT,
    ROOM_SELECTED,
    CHATTING,
    DEFAULT
};

struct Client
{
    int id;
    int socket;
    ClientState state;
    std::string username;
    int room_index;
};
