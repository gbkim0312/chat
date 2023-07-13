#include "chat_server.hpp"
#include <cstdlib>
#include "spdlog/fmt/fmt.h"
#include <cstdint>
#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fmt::print("usage: {} <PORT_NUM>", argv[0]); // NOLINT
        return 0;
    }

    int port = atoi(argv[1]); // NOLINT
    ChatServer chat_server(port);

    ServerState server_state = ServerState::STOP;
    uint8_t error_count = 0;

    // signal(SIGINT, signalHandler);

    while (true)
    {
        // Start server
        chat_server.start();
        server_state = chat_server.getState();

        if (server_state == ServerState::ERROR)
        {
            ++error_count;
            fmt::print("Server encountered an error. Restarting... ({} / 10)\n", error_count);
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
        fmt::print("Can not start server. try later. \n");
        chat_server.stop();
    }
    else if (server_state == ServerState::STOP)
    {
        fmt::print("Server stopped. \n");
        exit(0);
    }

    return 0;
}
