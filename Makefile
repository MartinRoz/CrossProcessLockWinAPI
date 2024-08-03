# Compiler
CC := g++

# Compiler flags
CFLAGS := -Wall -Wextra -pedantic -std=c++11

# Library name
LIBRARY := CrossProcessLock.a

# Source files
SRCS := $(wildcard *.cpp)

# Object files
OBJS := $(SRCS:.cpp=.o)

# Build rule for library
$(LIBRARY): $(OBJS)
	ar rcs $@ $^

# Build rule for object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(LIBRARY)