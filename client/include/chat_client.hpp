#pragma once

#include <memory>
#include <string>
#include "spdlog/fmt/fmt.h"
#include "chat_client.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

enum class ClientState
{
    CONNECTED,
    ROOM_LIST_RECEIVED, // <--> ROOM_LIST_SENT
    ROOM_SELECTED,
    ROOM_JOINED,
    CHATTING
};

class ChatClient
{
public:
    ChatClient(const std::string &server_ip, int port, const std::string &username);
    ~ChatClient();
    bool connectToServer();
    void start();

private:
    class ChatClientImpl;
    std::unique_ptr<ChatClientImpl> pimpl_;
};
