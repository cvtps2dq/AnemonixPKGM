cmake_minimum_required(VERSION 3.29)
project(Anemo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -g")

# Include directories
include_directories("/opt/local/include" "/opt/homebrew/include" src/core src/cli src/commands)

# Source files
file(GLOB_RECURSE CORE_SOURCES src/core/*.cpp)
file(GLOB_RECURSE COMMANDS_SOURCES src/commands/*.cpp)

# Find required libraries
find_package(LibArchive REQUIRED)
find_package(yaml-cpp REQUIRED)

# Create executable
add_executable(Anemo main.cpp ${CORE_SOURCES} ${COMMANDS_SOURCES}
        src/core/config.h
        src/cli/defines.h)

# Link libraries
target_link_libraries(Anemo archive sqlite3 yaml-cpp)