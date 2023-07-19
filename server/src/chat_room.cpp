#include "chat_room.hpp"
#include <string>
#include <utility>
#include "client.hpp"
#include <sys/socket.h>
#include <fmt/core.h>
#include <algorithm>
#include <utility>
#include <vector>

// state변경할 때 어떤 state에서 변경되었는지를

ChatRoom::ChatRoom(std::string name, int index) : name_(std::move(name)), index_(index) {}

void ChatRoom::addClient(Client client)
{
    clients_.push_back(std::move(client));
}

void ChatRoom::removeClient(const Client &client)
{
    clients_.erase(std::remove_if(clients_.begin(), clients_.end(), [&client](const Client &c)
                                  { return c.socket == client.socket; }),
                   clients_.end());
}

void ChatRoom::broadcastMessage(const std::string &message, const Client &sender, bool is_notice)
{
    std::string text;

    if (is_notice)
    {
        text = fmt::format("[NOTICE] : {}", message);
    }
    else
    {
        text = fmt::format("[{}] : {}", sender.username, message);
    }
    for (const auto &client : clients_)
    {
        if (client.socket != sender.socket)
        {
            auto send_bytes = send(client.socket, text.c_str(), text.size(), 0);
            // fmt::print("send message to {}\n", client.username);
            if (send_bytes < 0)
            {
                fmt::print("Failed to send message to clients");
            }
        }
    }
}

bool ChatRoom::sendParticipantsList(int client_socket)
{
    std::string client_list_str;

    for (auto client : clients_)
    {
        if (client.socket == client_socket)
        {
            client_list_str += fmt::format(" - {} (me)\n", client.username);
        }
        else
        {
            client_list_str += fmt::format(" - {}\n", client.username);
        }
    }

    auto send_bytes = send(client_socket, client_list_str.c_str(), client_list_str.size(), 0);
    return (send_bytes > 0);
}

void ChatRoom::setIndex(int index)
{
    index_ = index;
}

std::vector<Client> &ChatRoom::getClients()
{
    return clients_;
}

std::string ChatRoom::getName() const
{
    return name_;
}

int ChatRoom::getIndex() const
{
    return index_;
}

void ChatRoom::setOwner(Client client)
{
    owner_ = std::move(client);
}

Client ChatRoom::getOwner() const
{
    return owner_;
}
