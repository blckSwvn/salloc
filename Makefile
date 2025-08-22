# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Target executable
TARGET = main

# Source files
SRCS = main.c salloc.c
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Link object files
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
