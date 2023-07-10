#include "chat_client.hpp"
#include "spdlog/fmt/fmt.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fmt::print("usage: {} <SERVER_IP> <PORT>", argv[0]);
        exit(-1);
    }

    std::string serverIP = argv[1]; // 서버 IP 주소
    int port = std::stoi(argv[2]);  // 서버 포트 번호

    ChatClient client(serverIP, port);

    if (!client.connectToServer())
    {
        std::cerr << "Failed to connect to the server" << std::endl;
        return 1;
    }

    client.start();

    return 0;
}