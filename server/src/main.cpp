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
    ServerState server_state = ServerState::RUNNING;

    while (server_state == ServerState::RUNNING)
    {
        server_state = chatServer.start();
        if (server_state == ServerState::ERROR)
        {
            errorCount++;
            fmt::print("Server got error. Restarting...({} / 10)\n", errorCount);
            std::this_thread::sleep_for(std::chrono::seconds(6));
            if (errorCount == 10)
            {
                server_state = ServerState::STOP;
                // continue;
            }
        }
        if (server_state == ServerState::STOP)
        {
            fmt::print("Server stopped.\n");
            break;
        }
    }

    return 0;
}
