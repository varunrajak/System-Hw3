CC=gcc
CFLAGS=-Wall -Werror

all: hw3

hw3-1: hw3.c
	$(CC) $(CFLAGS) -o hw3 hw3.c

clean:
	rm -f hw3

