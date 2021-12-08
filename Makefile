CC=gcc
CFLAGS=-Wall -fsanitize=undefined
SRC=$(wildcard *.c)
LIBS=-ldl

tesh: $(SRC)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LIBS)