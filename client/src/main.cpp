#include <thread>
#include <chrono>
#include "chat_client.hpp"
#include "spdlog/fmt/fmt.h"

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fmt::print("usage: {} <USERNAME> <SERVER_IP> <PORT>", argv[0]);
        exit(-1);
    }

    std::string username = argv[1];
    std::string server_ip = argv[2];
    int32_t port = std::stoi(argv[3]);
    uint8_t error_count = 0;

    ChatClient client(server_ip, port, username);

    while (!client.connectToServer())
    {
        ++error_count;
        if (error_count > 20)
        {
            fmt::print("Connection timeout. exit program.\n");
            exit(-1);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fmt::print("Can not connect to the server. re-connecting...({} / 20)\n", error_count);
    }

    client.start();

    return 0;
}