CC = gcc
CFLAGS = -Iinclude -g -std=c11 -Wall -Wextra -Werror -D_GNU_SOURCE 
LDFLAGS = 
SRC = src/main.c src/token.c src/ast.c src/ir.c src/helper.c
OBJ = $(SRC:src/%.c=build/%.o)
OUT = build/main

# Default target
all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -static -O -o $(OUT) $(OBJ) $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f build/*.o $(OUT)
	clear

.PHONY: all clean