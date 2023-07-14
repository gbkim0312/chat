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
    std::shared_ptr<ChatRoom> findRoomByIndex(int index) const;
    const std::vector<ChatRoom> &getRooms();

private:
    std::vector<ChatRoom> rooms_;
    bool default_room_ = false;
};
