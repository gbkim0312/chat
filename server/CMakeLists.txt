project(server)

add_executable(${PROJECT_NAME}
    # include/chat_server.hpp
    # include/chat_room.hpp
    # include/chat_room_manager.hpp
    src/chat_server.cpp
    src/chat_room.cpp
    src/chat_room_manager.cpp
    src/client_handler.cpp
    src/main.cpp
)

find_package(spdlog REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
)

target_include_directories(${PROJECT_NAME}  PRIVATE include)
target_compile_features(${PROJECT_NAME}  PRIVATE cxx_std_14)

