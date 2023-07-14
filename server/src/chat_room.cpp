#include "chat_room.hpp"
#include <string>
#include <utility>
#include "client.hpp"
#include <sys/socket.h>
#include <fmt/core.h>
#include <algorithm>

ChatRoom::ChatRoom(std::string name, int index) : name_(std::move(name)), index_(index) {}

void ChatRoom::addClient(const Client &client)
{
    clients_.push_back(client);
}

void ChatRoom::removeClient(const Client &client)
{
    auto it = std::find_if(clients_.begin(), clients_.end(), [&client](const Client &c)
                           { return c.socket == client.socket; });

    if (it != clients_.end())
    {
        clients_.erase(it);
    }
}

void ChatRoom::broadcastMessage(const std::string &message, const Client &sender)
{
    const std::string text = fmt::format("[{}] : {}", sender.username, message);
    // for (const auto &client : clients_)
    // {
    //     fmt::print("send message to {}\n", client.username);
    // }
    for (const auto &client : clients_)
    {
        if (client.socket != sender.socket)
        {
            auto sendBytes = send(client.socket, text.c_str(), text.size(), 0);
            fmt::print("send message to {}\n", client.username);
            if (sendBytes < 0)
            {
                fmt::print("Failed to send message to clients");
            }
        }
    }
}

std::string ChatRoom::getName() const
{
    return name_;
}

int ChatRoom::getIndex() const
{
    return index_;
}
