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
    DISCONNECTED,
    DEFAULT
};

struct Client
{
    // int id = 0; //socket이 id 대신함, TODO: 연결종료 후 다른 client가 연결되었을 때 socket이 종료한 client와 겹치는 문제 발생가능
    int socket = -1;
    ClientState state = ClientState::DEFAULT;
    std::string username = "";
    int room_index = -1; // 참여중인 방 index // TODO: 방 삭제 시 인덱스 변하는 문제 있을수도..?
};
