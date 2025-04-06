/* hangman_server.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_CLIENTS 3
#define WORD_FILE "hangman_words.txt"
#define MAX_WORD_LENGTH 8
#define MAX_INCORRECT_GUESSES 6
#define MAX_INCOORECT_SIZE 12
#define PORT 8080

int active_clients = 0; 
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int socket;
    char *word;
    char guessed[MAX_WORD_LENGTH + 1];
    char incorrect[MAX_INCOORECT_SIZE];
    int incorrect_count;
} GameSession;

char *select_random_word() {
    FILE *file = fopen(WORD_FILE, "r");
    if (!file) {
        perror("Error opening word file");
        exit(EXIT_FAILURE);
    }

    char words[15][MAX_WORD_LENGTH + 1];
    int count = 0;
    
    printf("Debug: Reading words from %s...\n", WORD_FILE);
    while (fgets(words[count], MAX_WORD_LENGTH + 1, file) && count < 15) {
        words[count][strcspn(words[count], "\n")] = '\0';  // Remove newline
        if (strlen(words[count]) == 0) continue;  // Skip empty lines
        printf("Debug: Loaded word: %s\n", words[count]);
        count++;
    }
    fclose(file);

    if (count == 0) {
        fprintf(stderr, "Error: No valid words found in %s\n", WORD_FILE);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    int random_index = rand() % count;
    char *chosen_word = malloc(MAX_WORD_LENGTH + 1);
    if (!chosen_word) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    strncpy(chosen_word, words[random_index], MAX_WORD_LENGTH);
    chosen_word[MAX_WORD_LENGTH] = '\0';

    printf("Debug: Randomly selected word: %s\n", chosen_word);
    return chosen_word;
}

void *handle_client(void *arg) {
    GameSession *session = (GameSession *)arg;
    if (!session || !session->word) {
        fprintf(stderr, "Error: Session or word is NULL\n");
        pthread_exit(NULL);
    }
    
    char ready_buffer[10];
    int ready_bytes = recv(session->socket, ready_buffer, sizeof(ready_buffer) - 1, 0);
    if (ready_bytes <= 0) {
        close(session->socket);
        free(session->word);
        free(session);
        pthread_exit(NULL);
    }
    ready_buffer[ready_bytes] = '\0';

    if (strcmp(ready_buffer, "ready") != 0) {
        close(session->socket);
        free(session->word);
        free(session);
        pthread_exit(NULL);
    }

    printf("Debug: Chosen word = %s\n", session->word);

    session->incorrect_count = 0;
    for (int i = 0; i < strlen(session->word); i++) {
        session->guessed[i] = '_';
    }
    session->guessed[strlen(session->word)] = '\0';

    memset(session->incorrect, 0, MAX_INCORRECT_GUESSES + 1);

    printf("Debug: Sending initial game state: %s\n", session->guessed);

    // Send the initial masked word to the client
    char buffer[256];
    char display_mask[MAX_WORD_LENGTH * 2];
    int j = 0;
    for (int i = 0; i < strlen(session->guessed); i++) {
        display_mask[j++] = session->guessed[i];
        display_mask[j++] = ' ';
    }
    display_mask[j - 1] = '\0';  

    snprintf(buffer, sizeof(buffer), ">>>%s\n>>>Incorrect Guesses: %s\n>>>\n", display_mask, session->incorrect);
    send(session->socket, buffer, strlen(buffer), 0);

    while (session->incorrect_count < MAX_INCORRECT_GUESSES) {
        char guess;
        int recv_len = recv(session->socket, &guess, 1, 0);
        if (recv_len <= 0) break;  // Client disconnected, exit loop

        guess = tolower(guess);
        if (!isalpha(guess)) {
            send(session->socket, ">>>Error! Please guess one letter.\n", 35, 0);
            continue;
        }

        int correct = 0;
        for (int i = 0; i < strlen(session->word); i++) {
            if (session->word[i] == guess) {
                session->guessed[i] = guess;
                correct = 1;
            }
        }

        if (!correct) {
            int len = strlen(session->incorrect);
            session->incorrect_count++;
            if (len > 0) {
                session->incorrect[len] = ' ';
                session->incorrect[len + 1] = guess;
                session->incorrect[len + 2] = '\0';
            } else {
                session->incorrect[0] = guess;
                session->incorrect[1] = '\0';
            }
        }

        j = 0;
        for (int i = 0; i < strlen(session->guessed); i++) {
            display_mask[j++] = session->guessed[i];
            display_mask[j++] = ' ';
        }
        display_mask[j - 1] = '\0';

        if (strcmp(session->word, session->guessed) == 0) {
            snprintf(buffer, sizeof(buffer), ">>>The word was %s\n>>>You Win!\n>>>Game Over!\n", session->word);
            send(session->socket, buffer, strlen(buffer), 0);
            break;
        } else if (session->incorrect_count == MAX_INCORRECT_GUESSES) {
            snprintf(buffer, sizeof(buffer), ">>>The word was %s\n>>>You Lose!\n>>>Game Over!\n", session->word);
            send(session->socket, buffer, strlen(buffer), 0);
            break;
        } else {
            snprintf(buffer, sizeof(buffer), ">>>%s\n>>>Incorrect Guesses: %s\n>>>\n", display_mask, session->incorrect);
            send(session->socket, buffer, strlen(buffer), 0);
        }
    }

    // Decrement active client count when a client disconnects
    close(session->socket);
    free(session->word);
    free(session);

    pthread_mutex_lock(&client_mutex);
    active_clients--;
    pthread_mutex_unlock(&client_mutex);
    printf("Debug: Client disconnected. Active clients: %d\n", active_clients);

    pthread_exit(NULL);
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, MAX_CLIENTS);
    printf("Hangman Server listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);

        // Check if the server is full before accepting a new client
        pthread_mutex_lock(&client_mutex);
        if (active_clients >= MAX_CLIENTS) {
            pthread_mutex_unlock(&client_mutex);
            send(client_sock, ">>>server-overloaded\n", 21, 0);
            close(client_sock);
            printf("Debug: Overloaded client rejected. Active clients: %d\n", active_clients);
            continue;
        }

        send(client_sock, ">>>Welcome to Hangman!", 50, 0);

        active_clients++;  // Increment when a new client joins
        printf("Debug: New client connected. Active clients: %d\n", active_clients);
        pthread_mutex_unlock(&client_mutex);

        // Allocate and initialize game session
        GameSession *session = malloc(sizeof(GameSession));
        session->socket = client_sock;
        session->word = select_random_word();

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *)session);
        pthread_detach(thread);
    }

    close(server_sock);
    return 0;
}