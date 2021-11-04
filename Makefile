CC=gcc
CFLAGS=-Wall
SRC=$(wildcard *.c)
LIBS=-lreadline

tesh: $(SRC)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LIBS)