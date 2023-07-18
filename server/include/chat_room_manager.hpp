#pragma once

#include "chat_room.hpp"
#include <vector>
#include <memory>
#include <mutex>

class ChatRoomManager
{
public:
    void createRoom(const std::string &roomName, int roomIndex, const Client &owner);
    void createDefaultRooms(int n);
    void removeRoom(int room_index);
    ChatRoom &findRoomByIndex(int index);
    const std::vector<ChatRoom> &getRooms();

private:
    std::vector<ChatRoom> rooms_;
    std::mutex rooms_mutex_;
};
