# src/CMakeLists.txt

# Collect all source files recursively
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS
        "*.cpp"
        "cli/*.cpp"
        "commands/*.cpp"
)

# Create main executable
add_executable(anemonix ${SRCS})

# Include directories
target_include_directories(anemonix PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Target properties
set_target_properties(anemonix PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Link dependencies (found and configured in parent CMakeLists)
target_link_libraries(anemonix PRIVATE
        yaml-cpp
        OpenSSL::SSL
        sodium
)

# Installation configuration
install(TARGETS anemonix
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT runtime
)