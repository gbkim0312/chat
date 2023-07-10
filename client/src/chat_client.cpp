#include "chat_client.hpp"
#include "spdlog/fmt/fmt.h"

ChatClient::ChatClient(const std::string &serverIP, const int &port) : serverIP(serverIP), port(port), serverSocket(0), isRunning(false) {}

bool ChatClient::connectToServer()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        handleError("Fail to create the server socket");
        return false;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddress.sin_port = htons(port);

    if (connect(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == -1)
    {
        perror("Failed to connect to server");
        close(serverSocket);
        return false;
    }

    return true;
}

void ChatClient::start()
{
    isRunning = true;

    recvThread = std::thread(&ChatClient::recvMessages, this);
    sendThread = std::thread(&ChatClient::sendMessage, this);

    recvThread.join();
    sendThread.join();
}

void ChatClient::recvMessages()
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    while (isRunning)
    {
        ssize_t bytesRecv = recv(serverSocket, buffer, sizeof(buffer), 0);
        if (bytesRecv < 0)
        {
            handleError("Fail to receive message from the server");
            break;
        }
        else if (bytesRecv == 0)
        {
            handleError("Disconnected from the server");
            break;
        }
        else
        {
            fmt::print("Received: {}\n", std::string(buffer, sizeof(buffer)));
        }
        memset(buffer, 0, sizeof(buffer));
    }
}

void ChatClient::sendMessage()
{
    std::string message;

    while (isRunning)
    {
        std::getline(std::cin, message);
        if (message == "/quit")
        {
            break;
        }

        ssize_t bytesSent = send(serverSocket, message.c_str(), message.size(), 0);
        if (bytesSent == -1)
        {
            handleError("Failed to send message");
        }
    }
}

void ChatClient::handleError(const std::string &errorMessage)
{
    fmt::print("runtime error: {}\n", errorMessage);
    throw std::runtime_error(errorMessage);
}