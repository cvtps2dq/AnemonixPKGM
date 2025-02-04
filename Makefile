# Compiler
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -g

# Include directories
INCLUDES = -I/opt/local/include -I/opt/homebrew/include -Isrc/core -Isrc/cli -Isrc/commands

# Library directories
LDFLAGS = -L/opt/homebrew/lib

# Libraries
LIBS = -larchive -lsqlite3 -lyaml-cpp -lfmt

# Source files
CORE_SRCS = $(wildcard src/core/*.cpp)
COMMANDS_SRCS = $(wildcard src/commands/*.cpp)
SRCS = main.cpp $(CORE_SRCS) $(COMMANDS_SRCS)

# Output executable
TARGET = anemo

# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET) *.o