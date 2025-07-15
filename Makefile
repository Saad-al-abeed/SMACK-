# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -Wextra -pedantic -Iinclude $(shell pkg-config --cflags sdl2 SDL2_image SDL2_mixer SDL2_ttf)
LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf)

# Directories
SRC_DIR   := src
BUILD_DIR := build
INCLUDE_DIR := include

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET := $(BUILD_DIR)/SMACK!

# Default target
all: $(TARGET)

# Create build directory if needed
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean