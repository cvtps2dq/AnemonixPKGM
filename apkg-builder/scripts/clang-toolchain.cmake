set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Release)

# Security hardening flags
add_compile_options(
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -Wformat-security
)

# Linker hardening
add_link_options(
        -Wl,-z,relro,-z,now
)