#pragma once

#include <memory>
#include <string>

// enum class ClientState
// {
//     RUNNING,
//     ERROR,
//     STOP
// };

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
