# Compiler
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -g 

INCLUDES = -I/usr/local/include -I/usr/include -Isrc/core -Isrc/cli -Isrc/commands
LIBS = -larchive -lsqlite3 -lyaml-cpp -lfmt -lstdc++fs -lacl -lstdc++
LDFLAGS = -L/usr/local/lib

# Source files
CORE_SRCS = $(wildcard src/core/*.cpp)
COMMANDS_SRCS = $(wildcard src/commands/*.cpp)
SRCS = main.cpp $(CORE_SRCS) $(COMMANDS_SRCS)
DATABASE_SRC = $(wildcard src/database/*.cpp)
UTILS_SRC = $(wildcard src/utils/*.cpp)
FILESYSTEM_SRC = $(wildcard src/filesystem/*.cpp)

# Output executable
TARGET = anemo


# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(SRCS) $(DATABASE_SRC) $(UTILS_SRC) $(FILESYSTEM_SRC) -o $(TARGET) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET) *.o