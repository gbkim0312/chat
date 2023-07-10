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
    ChatClient(const std::string &serverIP, const int &port);
    bool connectToServer();
    void start();
    // void stop();

private:
    void recvMessages();
    void sendMessage();
    void handleError(const std::string &errorMessage);

    std::string serverIP;
    int port;
    int serverSocket;
    bool isRunning;
    std::thread recvThread;
    std::thread sendThread;
};
