#include <thread>
#include <chrono>
#include "chat_server.hpp"
#include "spdlog/fmt/fmt.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fmt::print("usage: {} <PORT_NUM>", argv[0]);
        exit(-1);
    }

    int port = atoi(argv[1]);
    ChatServer chatServer(port);
    uint8_t errorCount = 0;

    // start server
    while (true)
    {
        ServerState serverState = chatServer.start();

        if (serverState == ServerState::ERROR)
        {
            errorCount++;
            fmt::print("Server got error. Restarting...({} / 10)\n", errorCount);
            std::this_thread::sleep_for(std::chrono::seconds(6));
            if (errorCount == 10)
            {
                serverState = ServerState::STOP;
                // continue;
            }
        }
        if (serverState == ServerState::STOP)
        {
            fmt::print("Server stopped.\n");
            break;
        }
    }

    return 0;
}
