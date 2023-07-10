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
    // void stop();

private:
    void handleClient(const int &clientSocket);
    void broadcastMessage(const std::string &message, const ssize_t &messageSize, const int &senderSocket);
    void handleError(const std::string &errorMessage);

    int port;
    int serverSocket;
    std::vector<int> clientSockets;
    bool isRunning;
    std::mutex clientMutex;
};