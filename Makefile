CC = gcc
CFLAGS = -Wall -Werror -g

all: wish

wish: wish.c
	$(CC) $(CFLAGS) -o wish wish.c

clean:
	rm -f wish *.o

.PHONY: all clean