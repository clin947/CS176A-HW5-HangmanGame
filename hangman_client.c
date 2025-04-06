/* hangman_client.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    char buffer[256];

    // Wait for server to reply
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

    // if `recv()` fails then report initial error
    if (bytes_received <= 0) {
        perror("Error receiving initial message from server");
        close(sock);
        return 1;
    }

    buffer[bytes_received] = '\0';

    // if "server-overloaded" then report ocerloaded error
    if (strstr(buffer, "server-overloaded")) {
        printf("%s", buffer);
        close(sock);
        return 0;
    }

    // if not overloaded, ask "Ready?"
    char start;
    printf(">>>Ready to start game? (y/n): ");
    scanf(" %c", &start);
    getchar(); // Consume leftover newline character

    if (start != 'y' && start != 'Y') {
        printf("Exiting...\n");
        close(sock);
        return 0;
    }

    // send "ready" to server
    send(sock, "ready", 5, 0);

    // start game
    bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("Error receiving initial game state");
        close(sock);
        return 1;
    }

    buffer[bytes_received] = '\0';
    printf("%s", buffer);


    // Send Guesses and start game loop
    while (1) {
        char input[10];  // Allow space for invalid inputs (e.g., multiple letters)

        while (1) {
            printf(">>>Letter to guess: ");
            fflush(stdout);
            
            if (fgets(input, sizeof(input), stdin) == NULL) {  // Handle EOF (Ctrl-D)
                printf("\n");
                close(sock);
                return 0;
            }

            // Remove trailing newline
            input[strcspn(input, "\n")] = '\0';

            // Check for invalid input: Empty string, multiple characters, or non-letters
            if (strlen(input) != 1 || !isalpha(input[0])) {
                printf(">>>Error! Please guess one letter.\n");
                continue;  // Keep asking for input until valid
            }

            break;  // Valid input, exit loop
        }

        // Convert to lowercase and send to server
        char guess = tolower(input[0]);
        send(sock, &guess, 1, 0);

        // Receive game update
        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;
        buffer[bytes_received] = '\0';

        printf("%s", buffer);

        if (strstr(buffer, "Game Over!")) {
            break;
        }
    }

    close(sock);
    return 0;
}