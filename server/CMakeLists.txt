add_executable(server
    src/chat_server.cpp
    src/main.cpp
)

target_include_directories(server PRIVATE include)
target_compile_features(server PRIVATE cxx_std_14)

find_package(spdlog REQUIRED)
target_link_libraries(server PRIVATE spdlog::spdlog)