#include <memory>
#include <mutex>

enum class ServerState
{
    RUNNING,
    ERROR,
    STOP
};

class ChatServer
{
public:
    ChatServer(const int &port);
    ~ChatServer();

    void start();
    ServerState getState();
    void stop();

private:
    class ChatServerImpl;
    std::unique_ptr<ChatServerImpl> pimpl_;
};
