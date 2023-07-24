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

namespace network
{

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

        // TODO: 비효율적인것 같아서 다른 방법 생각 | 다른 클라이언트 간 동기화 문제 생각도 해봐야함 | Room ID 사용 고려
        // re-indexing room in the rooms_
        int new_index = -1;
        for (auto &room : rooms_)
        {
            room.setIndex(++new_index);
        }
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
        throw std::runtime_error("* Room not found.");
    }

    const std::vector<ChatRoom> &ChatRoomManager::getRooms()
    {
        return rooms_;
    }

    // create n default rooms
    void ChatRoomManager::createDefaultRooms(int n)
    {
        std::string room_name;

        for (int i = 0; i < n; i++)
        {
            room_name = fmt::format("Default Room {}", i);
            rooms_.push_back(ChatRoom(room_name, i));
        }

        fmt::println("* {} default rooms are created.", n);
    }
}