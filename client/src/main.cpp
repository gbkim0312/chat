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

    std::string server_ip = argv[2];   // 서버 IP 주소
    int32_t port = std::stoi(argv[3]); // 서버 포트 번호
    uint8_t error_counts = 0;

    ChatClient client(server_ip, port);

    while (!client.connectToServer())
    {
        if (error_counts > 20)
        {
            fmt::print("Connection timeout. exit program.\n");
            exit(-1);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        error_counts++;
        fmt::print("Can not connect to the server. re-connecting...({} / 20)\n", error_counts);
    }

    client.start();

    return 0;
}