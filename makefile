# Compiler
CC := gcc

# Project
TARGET := elsys_Tetris
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

# Find source files
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Detect OS
UNAME_S := $(shell uname -s)

# Platform-specific configuration
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)

    PLATFORM := windows
    RAYLIB_DIR := lib/windows

    CFLAGS := -I$(RAYLIB_DIR)
    LDFLAGS := -L$(RAYLIB_DIR) -lraylib -lgdi32 -lwinmm -mwindows

else ifeq ($(UNAME_S),Linux)

    PLATFORM := linux
    RAYLIB_DIR := lib/linux

    CFLAGS := -I$(RAYLIB_DIR)
    LDFLAGS := -L$(RAYLIB_DIR) -Wl,-Bstatic -lraylib -Wl,-Bdynamic -lm -lpthread -ldl -lrt -lX11

else

    $(error Unsupported OS: $(UNAME_S))

endif

# Default target
.PHONY: all
all: $(BIN_DIR)/$(TARGET)

# Link
$(BIN_DIR)/$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $@ ($(PLATFORM))..."
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Rebuild
.PHONY: rebuild
rebuild: clean all
