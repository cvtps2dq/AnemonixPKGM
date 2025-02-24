# Compiler
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -g

# Detect OS
OS := $(shell uname)

# Common includes (FIXED)
COMMON_INCLUDES = -I../../src/core -I../../src/commands -I../../src/cli

ifeq ($(OS), Linux)
    INCLUDES = -I/usr/local/include -I/usr/include $(COMMON_INCLUDES)
    LIBS = -larchive -lsqlite3 -lyaml-cpp -lfmt -lstdc++fs -lacl
    LDFLAGS = -L/usr/local/lib
endif

ifeq ($(OS), Darwin)
    INCLUDES = -I/opt/local/include -I/opt/homebrew/include $(COMMON_INCLUDES)
    LIBS = -larchive -lsqlite3 -lyaml-cpp -lfmt
    LDFLAGS = -L/opt/homebrew/lib
endif

# Source files
SRCS = transaction_tests.cpp ../../src/core/Database.cpp ../../src/core/Utilities.cpp ../../src/core/Anemo.cpp

# Output executable
TARGET = transaction-test

# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET) *.o