#pragma once

#include "chat_room.hpp"
#include <vector>
#include <memory>

class ChatRoomManager
{
public:
    // ChatRoomManager();
    void createRoom(const std::string &roomName, int roomIndex);
    void createDefaultRooms();
    std::unique_ptr<ChatRoom> findRoomByIndex(int index) const;
    std::vector<ChatRoom> getRooms() const;

private:
    std::vector<ChatRoom> rooms_;
    bool default_room_ = false;
};
