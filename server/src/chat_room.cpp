#include "chat_room.hpp"
#include <string>
#include <utility>
#include "client.hpp"
#include <fmt/core.h>
#include <algorithm>
#include <utility>
#include <vector>
#include "network_utility.hpp"

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

void ChatRoom::broadcastMessage(const std::string &message, const Client &sender, MessageType type)
{
    std::string text;

    switch (type)
    {
    case MessageType::NORMAL:
        text = fmt::format("[{}] : {}", sender.username, message);
        break;
    case MessageType::NOTICE:
        text = fmt::format("[NOTICE] : {}", message);
        break;
    case MessageType::COMMAND:
        text = fmt::format("{}", message);
        break;
    }

    for (const auto &client : clients_)
    {
        if (client.socket != sender.socket)
        {
            network::sendMessageToClient(client.socket, text);
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

    return network::sendMessageToClient(client_socket, client_list_str);
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
