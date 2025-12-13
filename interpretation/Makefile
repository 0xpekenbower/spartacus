CC = gcc
CFLAGS = -Wall -Wextra -g -O3
LDFLAGS = -lwebsockets -lcjson

SRC = src/main.c
OBJ = $(SRC:.c=.o)
TARGET = bin/interpretation

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
