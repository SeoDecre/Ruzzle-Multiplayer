# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Iinclude -c

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = include

# Source files
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/game_matrix.c $(SRC_DIR)/client.c

# Object files
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))

# Executable
EXEC = $(BIN_DIR)/main

# Libraries
LIBS = -lpthread

# Default target
all: $(EXEC)

# Linking the executable
$(EXEC): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ $(LIBS) -o $@

# Compiling source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Clean target to remove object files and executable
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
