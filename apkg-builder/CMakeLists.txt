cmake_minimum_required(VERSION 3.20)
project(AnemonixBuilder LANGUAGES CXX)

find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(sodium REQUIRED)

add_executable(apkg-builder
        src/main.cpp
        src/PackageBuilder.cpp
)

target_link_libraries(apkg-builder
        PRIVATE yaml-cpp OpenSSL::SSL sodium
)

target_compile_features(apkg-builder PRIVATE cxx_std_23)