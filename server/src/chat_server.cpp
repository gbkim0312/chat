#include "chat_server.hpp"
#include "spdlog/fmt/fmt.h"

ChatServer::ChatServer(const int &port) : port(port), serverSocket(0), isRunning(false) {}

ServerState ChatServer::start()
{
    // create the server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        handleError("Fail to create the server socket");
        return ServerState::ERROR;
    };

    // set the address of the socket
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // binding the socket and the address
    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == -1)
    {
        handleError("Failed to bind socket");
        return ServerState::ERROR;
    }

    // listen clients
    if (listen(serverSocket, 1) == -1)
    {
        handleError("Fail to listen for connection");
        return ServerState::ERROR;
    }

    isRunning = true;
    fmt::print("Server started. Listening on port {} \n", port);

    while (isRunning)
    {
        // accept clients
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0)
        {
            handleError("Failed to accept clinet connection");
            continue;
        }

        fmt::print("Client connected. Socket FD: {} \n", clientSocket);

        // push_back the client socket to clientSockets.
        clientSockets.push_back(clientSocket);

        // start clientHandler thread
        std::thread clientThread(&ChatServer::handleClient, this, clientSocket);
        clientThread.detach(); // 스레드를 분리해서 백그라운드에서 실행
    }

    isRunning = false;
    return ServerState::STOP;
}

void ChatServer::handleClient(const int &clientSocket)
{
    // create buffer for message and clear it
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // receive messages
    while (isRunning)
    {
        ssize_t bytesRecv = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRecv < 0)
        {
            handleError("Failed to receive message from the client");
            break;
        }
        else if (bytesRecv == 0)
        {
            fmt::print("Client disconnected. Socket FD: {}", clientSocket);
            break;
        }
        else
        {
            broadcastMessage(std::string(buffer, bytesRecv), bytesRecv, clientSocket);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    close(clientSocket);

    // remove the clientSocket.
    // TODO: need to add mutex
    clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
}

void ChatServer::broadcastMessage(const std::string &message, const ssize_t &messageSize, const int &senderSocket)
{
    for (int clientSocket : clientSockets)
    {
        if (clientSocket != senderSocket)
        {
            ssize_t sendBytes = send(clientSocket, message.c_str(), messageSize, 0);
            if (sendBytes < 0)
            {
                handleError("Failed to send message to clients");
            }
        }
    }
}

void ChatServer::handleError(const std::string &errorMessage)
{
    fmt::print("runtime error: {}\n", errorMessage);
    throw std::runtime_error(errorMessage);
}