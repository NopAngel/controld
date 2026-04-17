# Makefile - Build system for controld
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=gnu11 -ggdb
SRC_DIR = src
OBJ = controld controlctl

all: $(OBJ)

controld: $(SRC_DIR)/controld.c
	$(CC) $(CFLAGS) -o $@ $<

controlctl: $(SRC_DIR)/controlctl.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJ) /tmp/controld.sock

.PHONY: all clean