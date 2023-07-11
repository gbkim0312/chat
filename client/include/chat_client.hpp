#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <memory>
#include <string>

enum class ClientState
{
    RUNNING,
    ERROR,
    STOP
};

class ChatClient
{
public:
    ChatClient(const std::string &server_ip, const int &port);
    ~ChatClient();
    bool connectToServer();
    void start();

private:
    class ChatClientImpl;
    std::unique_ptr<ChatClientImpl> pimpl_;
};

#endif // CHAT_CLIENT_HPP
