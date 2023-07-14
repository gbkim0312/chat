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

void ChatRoomManager::createRoom(const std::string &roomName, int roomIndex)
{
    const ChatRoom room(roomName, roomIndex);
    rooms_.push_back(room);
}

std::unique_ptr<ChatRoom> ChatRoomManager::findRoomByIndex(int index) const
{
    for (auto const &room : rooms_)
    {
        if (room.getIndex() == index)
        {
            return std::make_unique<ChatRoom>(room);
        }
    }
    return nullptr;
}

std::vector<ChatRoom> ChatRoomManager::getRooms() const
{
    return rooms_;
}