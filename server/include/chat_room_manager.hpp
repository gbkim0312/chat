#pragma once

#include "chat_room.hpp"
#include <vector>
#include <memory>
#include <mutex>

class ChatRoomManager
{
public:
    // ChatRoomManager();
    void createRoom(const std::string &roomName, int roomIndex, const Client &owner);
    void createDefaultRooms();
    void removeRoom(int room_index);
    ChatRoom &findRoomByIndex(int index);
    const std::vector<ChatRoom> &getRooms();

private:
    std::vector<ChatRoom> rooms_;
    bool default_room_ = false;
    std::mutex rooms_mutex_;
};
