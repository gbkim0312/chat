#pragma once

#include "client.hpp"
#include <vector>
#include <string>
#include <mutex>
class ChatRoom
{
public:
    ChatRoom(std::string name, int index);

    void addClient(Client client);
    void removeClient(const Client &client);
    void broadcastMessage(const std::string &message, const Client &sender, bool is_notice = false);

    std::string getName() const;
    std::vector<Client> getClients() const;
    int getIndex() const;
    void setOwner(Client client);
    Client getOwner() const;
    void setIndex(int index);

private:
    std::vector<Client> clients_;
    std::string name_;
    int index_;
    // std::mutex clients_mutex_;
    Client owner_;
};
