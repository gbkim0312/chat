#pragma once

#include "client.hpp"
#include <vector>
#include <string>

class ChatRoom
{
public:
    ChatRoom(std::string name, int index);

    void addClient(Client &client);
    void removeClient(const Client &client);
    void broadcastMessage(const std::string &message, const Client &sender);

    std::string getName() const;
    int getIndex() const;

private:
    std::vector<Client> clients_;
    std::string name_;
    int index_;
};
