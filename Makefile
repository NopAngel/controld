# Makefile - Build system for controld
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=gnu11 -ggdb
SRC_DIR = src
OBJ = controld controlctl
Q = @
all: $(OBJ)

controld: $(SRC_DIR)/controld.c
	$(Q)$(CC) $(CFLAGS) -o $@ $<
	$(Q)echo " CC  $<"

controlctl: $(SRC_DIR)/controlctl.c
	$(Q)$(CC) $(CFLAGS) -o $@ $<
	$(Q)echo " CC  $<"

clean:
	$(Q)rm -f $(OBJ) /tmp/controld.sock
	$(Q)echo " Done. "

.PHONY: all clean
