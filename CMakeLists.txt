
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/module" ${CMAKE_MODULE_PATH})
message(STATUS "${CMAKE_MODULE_PATH}")
include(V2XClangTidyFlags)




cmake_minimum_required(VERSION 3.10)
project(ChatProject)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # clang-tidy를 통해 정적 분석 수행
  option(CLANG_TIDY_FIX "Perform fixes for Clang-Tidy" OFF)
  find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "Path to clang-tidy executable"
  )
  find_program( # clang이 사용되는 것은 아니나, clang-tidy의 올바른 동작을 위해 필요함
    CLANG_EXE
    NAMES "clang"
    DOC "Path to clang executable"
  )

  # 정적 분석 준비
  if((NOT CLANG_TIDY_EXE) OR (NOT CLANG_EXE))
    message(FATAL_ERROR "clang-tidy tool not found")
  endif()
endif()

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