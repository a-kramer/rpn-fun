CC = gcc
CFLAGS = -O2 -march=native -std=gnu23

.PHONY: all


all: rpnc


rpnc: main.c
	$(CC) $(CFLAGS) -Wall -o $@ $^ -lm
