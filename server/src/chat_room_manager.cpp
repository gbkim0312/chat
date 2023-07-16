#include "chat_room_manager.hpp"
#include "chat_room.hpp"
#include <string>
#include <memory>
#include <vector>
#include <utility>

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
    auto room = std::make_unique<ChatRoom>(name, index);
    rooms_.push_back(std::move(room));
}

std::shared_ptr<ChatRoom> ChatRoomManager::findRoomByIndex(int index)
{
    for (auto const &room : rooms_)
    {
        if (room->getIndex() == index)
        {
            return room;
        }
    }
    return nullptr;
}
// Reference 전달

const std::vector<std::shared_ptr<ChatRoom>> &ChatRoomManager::getRooms()
{
    return rooms_;
}
// Reference 전달