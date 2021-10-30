CC=gcc
CFLAGS=-Wall
SRC=$(wildcard *.c)
LIBS=-lreadline

tesh: $(SRC)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
