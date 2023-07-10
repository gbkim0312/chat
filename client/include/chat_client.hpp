#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

enum class ClientState
{
    RUNNING,
    ERROR,
    STOP
};

class ChatClient
{
public:
    ChatClient(const std::string &server_ip, const int &port_);
    bool connectToServer();
    void start();

private:
    void recvMessages();
    void sendMessage();
    void handleError(const std::string &error_message);
    sockaddr_in server_addr_{};

    std::string server_ip_;
    int port_;
    int server_socket_ = 0;
    bool is_running_ = false;
    std::thread recv_thread_;
    std::thread send_thread_;
};
