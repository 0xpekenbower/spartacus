CC = gcc
CFLAGS = -Wall -Wextra -g -O3 -march=native -mavx2
LDFLAGS = -lwebsockets -lcjson -lssl -lcrypto -lcurl -I/usr/local/include/ta-lib -lta-lib -lm

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Find all .c files recursively
SRCS = $(shell find $(SRC_DIR) -name '*.c')
# Create object file paths preserving directory structure
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/collector

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Rule to compile .c files to .o files, creating necessary directories
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
