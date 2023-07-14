project(client)

add_executable(${PROJECT_NAME}
    #include/chat_client.hpp
    src/chat_client.cpp
    src/main.cpp
)

find_package(spdlog REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

