CC = gcc
CFLAGS=-O2 -Wall -Wextra -std=c11 -Iinclude
LDFLAGS=
BUILD=build

SRC=$(wildcard src/*.c)
OBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

all: $(BUILD)/marqdb

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/marqdb: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD)
