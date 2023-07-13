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
