CC = gcc
CFLAGS = -Wall -pthread

all: hangman_server hangman_client

hangman_server: hangman_server.c
	$(CC) $(CFLAGS) hangman_server.c -o hangman_server

hangman_client: hangman_client.c
	$(CC) $(CFLAGS) hangman_client.c -o hangman_client

clean:
	rm -f hangman_server hangman_client
