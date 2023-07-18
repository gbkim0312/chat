#include "chat_room_manager.hpp"
#include "chat_room.hpp"
#include "client.hpp"
#include <string>
#include <stdexcept>
#include <vector>
#include <utility>
#include <mutex>
#include <algorithm>
#include <fmt/core.h>

void ChatRoomManager::createRoom(const std::string &name, int index, const Client &owner)
{
    auto room = ChatRoom(name, index);
    room.setOwner(owner);
    std::lock_guard<std::mutex> const lock_guard(rooms_mutex_);
    rooms_.push_back(std::move(room));
}

void ChatRoomManager::removeRoom(int room_index)
{
    rooms_.erase(std::remove_if(rooms_.begin(), rooms_.end(), [room_index](const ChatRoom &chat_room)
                                { return chat_room.getIndex() == room_index; }),
                 rooms_.end());
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
    throw std::runtime_error("Room not found.");
}

const std::vector<ChatRoom> &ChatRoomManager::getRooms()
{
    return rooms_;
}

void ChatRoomManager::createDefaultRooms()
{
    std::string room_name;

    for (int i = 0; i < 4; i++)
    {
        room_name = fmt::format("Default Room {}", i);
        rooms_.push_back(ChatRoom(room_name, i));
    }
}