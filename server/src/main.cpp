#include "chat_server.hpp"
#include <cstdlib>
#include <fmt/core.h>
#include <cstdint>
#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fmt::println("usage: {} <PORT_NUM>", argv[0]); // NOLINT
        return 0;
    }

    int port = std::stoi(argv[1]); // NOLINT
    ChatServer chat_server(port);

    ServerState server_state = ServerState::STOP;
    uint8_t error_count = 0;

    while (true)
    {
        // Start server
        chat_server.start();
        server_state = chat_server.getState();

        if (server_state == ServerState::ERROR)
        {
            ++error_count;
            fmt::println("* Server encountered an error. Restarting... ({} / 10)", error_count);
            std::this_thread::sleep_for(std::chrono::seconds(6));
            if (error_count > 10)
            {
                break;
            }
            continue;
        }
        else if (server_state == ServerState::STOP)
        {
            break;
        }
    }

    if (server_state == ServerState::ERROR)
    {
        fmt::println("* Can not start server. try later. \n");
        chat_server.stop();
    }
    else if (server_state == ServerState::STOP)
    {
        fmt::println("* Server stopped. \n");
        exit(0);
    }

    return 0;
}
