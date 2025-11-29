CC = gcc
CFLAGS = -pthread

all: client server

client: client.c common.h
	$(CC) $(CFLAGS) -o client client.c common.c

server: server.c common.h
	$(CC) $(CFLAGS) -o server server.c common.c

clean:
	rm -f client server

.PHONY: all clean