#pragma once

#include "client.hpp"
#include <vector>
#include <string>
#include <mutex>

// TODO: enum Class MessageType 정의 후, broadcastMessage
namespace network
{

    enum class MessageType
    {
        NORMAL,
        NOTICE,
        COMMAND
    };

    class ChatRoom
    {
    public:
        ChatRoom(std::string name, int index);

        void addClient(Client client);
        void removeClient(const Client &client);
        void broadcastMessage(const std::string &message, const Client &sender, MessageType type = MessageType::NORMAL);
        bool sendParticipantsList(int client_socket);

        std::string getName() const;
        std::vector<Client> &getClients();
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
}