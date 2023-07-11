add_executable(client
    src/chat_client.cpp
    src/main.cpp
)

target_include_directories(client PRIVATE include)
target_compile_features(client PRIVATE cxx_std_14)

find_package(spdlog REQUIRED)
target_link_libraries(client PRIVATE spdlog::spdlog)