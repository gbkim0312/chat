#pragma once

#include "client.hpp"
#include <vector>
#include <string>

class ChatRoom
{
public:
    ChatRoom(std::string name, int index);

    void addClient(Client client);
    void removeClient(const Client &client);
    void broadcastMessage(const std::string &message, const Client &sender, bool notice);

    std::string getName() const;
    int getIndex() const;

private:
    std::vector<Client> clients_;
    std::string name_;
    int index_;
    // TODO: Client owner; - 생성한사람이 지울 수 있도록
};
