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
    ChatServer chat_server(port);
    ServerState server_state;
    int errorCounts = 0;

    while (true)
    {
        // Start server
        chat_server.start();
        server_state = chat_server.getState();

        if (server_state == ServerState::ERROR)
        {
            errorCounts++;
            fmt::print("Server encountered an error. Restarting... ({} / 10)\n", errorCounts);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (errorCounts > 9)
            {
                break;
            }
            continue;
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
    }

    return 0;
}
