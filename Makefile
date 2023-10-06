CC=gcc
SRC=proxy.c
OBJ=proxy

all: $(OBJ)

proxy: $(SRC)
	$(CC) $< -o $@

clean:
	rm -rf $(OBJ)

.PHONY: all clean
