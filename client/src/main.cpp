#include "chat_client.hpp"
#include "spdlog/fmt/fmt.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fmt::print("usage: {} <SERVER_IP> <PORT>", argv[0]);
        exit(-1);
    }

    std::string server_ip = argv[1]; // 서버 IP 주소
    int port = std::stoi(argv[2]);   // 서버 포트 번호

    ChatClient client(server_ip, port);

    if (!client.connectToServer())
    {
        fmt::print("Failed to connect to the server\n");
        return 1;
    }

    client.start();

    return 0;
}