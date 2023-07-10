#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
    ServerState start();

private:
    void handleClient(const int &client_socket);
    void broadcastMessage(const std::string &message, const ssize_t &message_size, const int &sender_socket);
    void handleError(const std::string &error_message);

    sockaddr_in server_addr_{};
    int port_ = 0;
    int server_socket_ = 0;
    std::vector<int> client_sockets_;
    bool is_running_ = false;
    std::mutex client_mutex_;
};