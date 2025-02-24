# Compiler
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -g

# OS-specific settings
OS := $(shell uname)

ifeq ($(OS), Linux)
    # Linux-specific includes and libraries
    INCLUDES = -I/usr/local/include -I/usr/include -Isrc/core -Isrc/cli -Isrc/commands
    LIBS = -larchive -lsqlite3 -lyaml-cpp -lfmt -lstdc++fs -lacl
    LDFLAGS = -L/usr/local/lib
endif

ifeq ($(OS), Darwin)
    # macOS-specific includes and libraries
    INCLUDES = -I/opt/local/include -I/opt/homebrew/include -I../src/core -I../src/cli -I../src/commands
    LIBS = -larchive -lsqlite3 -lyaml-cpp -lfmt
    LDFLAGS = -L/opt/homebrew/lib
endif

SRCS = parsing.cpp

# Output executable
TARGET = test-parser

# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET) *.o