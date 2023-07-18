#include "chat_room_manager.hpp"
#include "chat_room.hpp"
#include <string>
#include <stdexcept>
// #include <memory>
#include <vector>
#include <utility>
#include <mutex>
// #include <fmt/core.h>

void ChatRoomManager::createDefaultRooms()
{
    if (default_room_)
    {
        createRoom("Room 1", 0);
        createRoom("Room 2", 1);
        createRoom("Room 3", 2);
    }
}

void ChatRoomManager::createRoom(const std::string &name, int index)
{
    auto room = ChatRoom(name, index);
    std::lock_guard<std::mutex> const lock_guard(rooms_mutex_);
    rooms_.push_back(std::move(room));
}

ChatRoom &ChatRoomManager::findRoomByIndex(int index)
{
    std::lock_guard<std::mutex> const lock_guard(rooms_mutex_);
    for (auto &room : rooms_)
    {
        if (room.getIndex() == index)
        {
            return room;
        }
    }
    // Throw room not found exception
    throw std::runtime_error("Room not found. Choose again");
}
// Reference 전달

const std::vector<ChatRoom> &ChatRoomManager::getRooms()
{
    return rooms_;
}
