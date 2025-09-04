cmake_minimum_required(VERSION 3.16)
project(tirapiedras_cpp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally allow static linking of raylib if available
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

find_package(raylib QUIET)

if(NOT raylib_FOUND)
  message(STATUS "raylib not found via find_package. Fetching via FetchContent...")
  include(FetchContent)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 48eac6017163a31d143b56a21211b79c8fbf664b
  )
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(raylib)
endif()

add_executable(tirapiedras
  src/main.cpp
)

target_link_libraries(tirapiedras PRIVATE raylib)

if (MSVC)
  target_compile_options(tirapiedras PRIVATE /W4)
else()
  target_compile_options(tirapiedras PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS tirapiedras)

