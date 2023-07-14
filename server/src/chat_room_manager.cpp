#include "chat_room_manager.hpp"
#include "chat_room.hpp"
#include <string>
#include <memory>
#include <vector>

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
    const ChatRoom room(name, index);
    rooms_.push_back(room);
}

std::shared_ptr<ChatRoom> ChatRoomManager::findRoomByIndex(int index) const
{
    for (auto const &room : rooms_)
    {
        if (room.getIndex() == index)
        {
            return std::make_shared<ChatRoom>(room);
        }
    }
    return nullptr;
}

const std::vector<ChatRoom> &ChatRoomManager::getRooms()
{
    return rooms_;
}