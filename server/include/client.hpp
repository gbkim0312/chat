#pragma once
#include <map>
#include <string>
#include <functional>

class ChatRoomManager;

enum class ClientState
{
    CONNECTED,
    OPTION_SENT,
    OTPION_SELECTED,
    CHATTING,
    LEAVING,
    DISCONNECTED,
    DEFAULT
};

enum class ClientTrigger
{
    SEND_ROOMS,
    RECV_OPTION,
    JOIN_ROOM,
    CREATE_ROOM,
    REMOVE_ROOM,
    START_CHAT,
    LEAVE,
    DISCONNECT
};

struct Client
{
    // int id = 0; //socket이 id 대신함, TODO: 연결종료 후 다른 client가 연결되었을 때 socket이 종료한 client와 겹치는 문제 발생가능
    int socket = -1;
    ClientState state = ClientState::DEFAULT;
    std::string username = "";
    int room_index = -1; // 참여중인 방 index // TODO: 방 삭제 시 인덱스 변하는 문제 있을수도..?
};