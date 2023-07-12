set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/module" ${CMAKE_MODULE_PATH})
message(STATUS "${CMAKE_MODULE_PATH}")
include(V2XClangTidyFlags)

cmake_minimum_required(VERSION 3.10)
project(ChatProject)

# CMAKE_BUILD_TYPE 관련 옵션은 g++ 기본 설정을 따름
# Default: None
# Debug: -g (본 프로젝트의 기본 옵션)
# Release -O3 DNDEBUG
# MinSizeRel: -Os DNDEBUG
# RelWithDebInfo: -O2 -g DNDEBUG

# 모듈 패스 설정


# cmake 모듈 include


add_subdirectory(server)
add_subdirectory(client)