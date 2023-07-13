#pragma once

#include <string>
#include "spdlog/fmt/fmt.h"

enum class ClientState
{
    CONNECTED,      // 클라이언트 접속
    ROOM_LIST_SENT, // 채팅방 리스트 전송 완료
    ROOM_SELECTED,  // 채팅방 선택 완료
    ROOM_JOINED,    // 채팅방 접속 완료
    CHATTING        // 채팅 중
};
struct Client
{
    int id;
    int socket;
    std::string username;
    ClientState state;
};

enum class ServerState
{
    RUNNING,
    ERROR,
    STOP
};

class ChatServer
{
public:
    ChatServer(int port);
    ~ChatServer();

    void start();
    ServerState getState();
    void stop();
    void setPort(int port);

private:
    class ChatServerImpl;
    std::unique_ptr<ChatServerImpl> pimpl_;
};
