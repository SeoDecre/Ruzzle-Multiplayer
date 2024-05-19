#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/select.h>
#include <time.h>
#include "include/macros.h"
#include "include/game_matrix.h"
#include "include/game_players.h"

void server(int port, Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    PlayerList* playersList = createPlayerList();
    int server_fd, retvalue, max_fd, new_client_fd;
    fd_set readfds;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[MATRIX_BYTES + sizeof(int)];

    // Inizializzazione della struttura server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 127.0.0.1

    // Creazione del socket
    SYSC(server_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Binding
    SYSC(retvalue, bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella bind");

    // Listen
    SYSC(retvalue, listen(server_fd, MAX_CLIENTS), "nella listen");

    printf("Ready to listen.\nGenerated matrix:\n");
    printMatrix(matrix);

    // Settaggio del massimo descrittore
    max_fd = server_fd;

    // Main loop
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        // Aggiunta dei descrittori dei client alla set dei descrittori
        for (int i = 0; i < playersList->size; i++) {
            if (playersList->players[i].fd != -1) FD_SET(playersList->players[i].fd, &readfds);
        }

        // Select
        SYSC(retvalue, select(max_fd + 1, &readfds, NULL, NULL, NULL), "nella select");

        // Accettazione di nuovi client
        if (FD_ISSET(server_fd, &readfds)) {
            client_addr_len = sizeof(client_addr);
            SYSC(new_client_fd, accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len), "nell'accept");
            
            printf("Client accepted, socket fd is %d\n", new_client_fd);

            // Add new player to the list
            addPlayer(playersList, (Player){new_client_fd, -1, "null", 0});

            printPlayerList(playersList);

            // Aggiunta del nuovo client ai descrittori dei client
            for (int i = 0; i < playersList->size; i++) {
                // if (playersList->players[i].fd == -1) {
                    // playersList->players[i].fd = new_client_fd;

                    // Receive nickname
                    char nickname[MAX_NICKNAME_LENGTH];
                    retvalue = read(playersList->players[i].fd, nickname, sizeof(nickname));
                    nickname[retvalue] = '\0';
                    FD_SET(playersList->players[i].fd, &readfds);

                    // Check if nickname already exists
                    int nickname_exists = !nicknameAlreadyExists(playersList, nickname);

                    // Send validation response to client
                    char validation_response[2];
                    validation_response[0] = nickname_exists ? '0' : '1';
                    validation_response[1] = '\0';
                    SYSC(retvalue, write(playersList->players[i].fd, validation_response, sizeof(validation_response)), "in write");

                    if (nickname_exists) {
                        printf("already exists");
                        continue; // Client needs to choose a different nickname
                    }
                    printf("doesn't exist");

                    // Else just update his record
                    updatePlayerNickname(playersList, playersList->players[i].fd, nickname);

                    printPlayerList(playersList);

                    // Prepare buffer to send matrix and score
                    int score = 0;
                    memcpy(buffer, matrix, MATRIX_BYTES);
                    memcpy(buffer + MATRIX_BYTES, &score, sizeof(int));

                    // Send the buffer to the client
                    write(playersList->players[i].fd, buffer, MATRIX_BYTES + sizeof(int));

                    // break;
                // }
            }

            // Aggiornamento del massimo descrittore
            if (new_client_fd > max_fd) max_fd = new_client_fd;
        }

        // Handle client activity (must be outside of the previous if statement)
        for (int i = 0; i < playersList->size; i++) {
            new_client_fd = playersList->players[i].fd;
            if (FD_ISSET(new_client_fd, &readfds)) {
                retvalue = read(new_client_fd, buffer, sizeof(buffer));
                if (retvalue == 0) {
                    // Client disconnected
                    printf("Client disconnected, socket fd was %d\n", new_client_fd);
                    removePlayer(playersList, playersList->players[i].fd);
                    close(new_client_fd);
                } else {
                    // Echo back to client
                    buffer[retvalue] = '\0';
                    printf("Received from client %d: %s\n", new_client_fd, buffer);
                    if (strcmp(buffer, "P")) printPlayerList(playersList);
                }
            }
        }
    }

    // Chiusura del socket del server (non raggiungibile)
    SYSC(retvalue, close(server_fd), "nella close");
}

int main(int argc, char *argv[]) {
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];
    char word[MAX_WORD_LENGTH];
    int found = 0, score = 0, i, j;

    // Seed the random number generator with current time
    srand(time(NULL));

    // Filling it with random letters and setting all the cell colors to white
    initMatrix(matrix);

    // Port parsing
    int port = atoi(argv[1]);
    printf("Server started at localhost:%d\n", port);
    
    server(port, matrix);
    
    /*
    while (1) {
        // Clear the screen and move cursor to top-left
        printf("\033[2J"); // ANSI escape sequence to clear screen
        printf("\033[H");  // ANSI escape sequence to move cursor to top-left

        // Print the matrix
        printMatrix(matrix);
        
        // Print the current score
        printf("Score: %d\n", score);
        printf(">> ");

        // Read user input
        if (scanf("%s", word) != 1) {
            printf("Error reading input.\n");
            return -1;
        }

        if (strlen(word) >= 4) {
            // Search for the source cells (first letter of the word)
            for (i = 0; i < MATRIX_SIZE; i++) {
                for (j = 0; j < MATRIX_SIZE; j++) {
                    if (matrix[i][j].letter == word[0]) {
                        // Perform DFS to check for the word
                        checkMatrixWord(matrix, &found, word, 0, i, j);
                        // Clean the matrix for next search
                        cleanMatrix(matrix);
                    }
                }
            }
        } else {
            printf("Word is too short\n");
        }

        // Update score and display appropriate message
        if (found) {
            score += strlen(word);
            printf("Found \"%s\"!\n", word);
        } else {
            printf("Word not found.\n");
        }

        found = 0;
    }
    */
    return 0;
}