CC = gcc
CFLAGS = -Wall -Wextra -g -O3 -march=native -mavx2
LDFLAGS = -I/usr/local/include/ta-lib -lta-lib -lm
LDFLAGS_DL = -lcurl -lzip -lm

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Find all .c files recursively
SRCS_HIST = src/main.c src/calc/indicators.c src/parser/csv_parser.c
OBJS_HIST = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRCS_HIST))

SRCS_DL = src/download_data.c
OBJS_DL = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRCS_DL))

TARGET_HIST = $(BIN_DIR)/historical
TARGET_DL = $(BIN_DIR)/download-data

.PHONY: all clean

all: $(TARGET_HIST) $(TARGET_DL)

$(TARGET_HIST): $(OBJS_HIST) | $(BIN_DIR)
	$(CC) $(OBJS_HIST) -o $@ $(LDFLAGS)

$(TARGET_DL): $(OBJS_DL) | $(BIN_DIR)
	$(CC) $(OBJS_DL) -o $@ $(LDFLAGS_DL)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
