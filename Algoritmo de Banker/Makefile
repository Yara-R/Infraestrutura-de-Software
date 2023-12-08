CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: banker

banker: banker.o
	$(CC) $(CFLAGS) -o banker banker.o

banker.o: banker.c
	$(CC) $(CFLAGS) -c banker.c

clean:
	rm -f banker banker.o
