cmake_minimum_required(VERSION 3.20)
project(AnemonixCLI LANGUAGES CXX)

# Dependency configuration
find_package(yaml-cpp REQUIRED)

# Configure compiler and standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source groups for better IDE organization
add_subdirectory(src)

# Main executable
add_executable(anemonix
        src/main.cpp

        # Core components
        src/cli/cli.cpp
        src/package.cpp

        # Command implementations
        src/commands/install.cpp
        src/commands/build.cpp
        src/commands/search.cpp
        src/commands/remove.cpp
)

# Include directories
target_include_directories(anemonix PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(anemonix PRIVATE
        yaml-cpp
        OpenSSL::SSL
        sodium
)

# Install target
install(TARGETS anemonix DESTINATION bin)