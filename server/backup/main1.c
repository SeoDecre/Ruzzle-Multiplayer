#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

// Struct to represent a player
typedef struct {
    int fd;
    pthread_t tid;
    char name[MAX_NICKNAME_LENGTH];
    int score;
} Player;

// Struct to manage a list of players
typedef struct {
    Player *players;
    int size;
    int capacity;
} PlayersList;

PlayersList playersList; // Global players list

// Function prototypes
void *handle_client(void *arg);

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int i = 0;

    // Initialize players list
    playersList.players = (Player *)malloc(MAX_CLIENTS * sizeof(Player));
    playersList.size = 0;
    playersList.capacity = MAX_CLIENTS;

    // Create socket
    SYSC(server_fd, socket(AF_INET, SOCK_STREAM, 0), "Socket creation failed");

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to the given address and port
    SYSC(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "Bind failed");

    // Listen for incoming connections
    SYSC(listen(server_fd, MAX_CLIENTS), "Listen failed");

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connection
        client_len = sizeof(client_addr);
        SYSC(client_fd, accept(server_fd, (struct sockaddr *)&client_addr, &client_len), "Accept failed");

        printf("New client connected\n");

        // Create a new thread to handle the client
        Player *newPlayer = &playersList.players[playersList.size];
        newPlayer->fd = client_fd;
        SYSC(pthread_create(&newPlayer->tid, NULL, handle_client, (void *)newPlayer), "Thread creation failed");

        // Update players list
        playersList.size++;

        // Check if players list capacity needs to be increased
        if (playersList.size >= playersList.capacity) {
            playersList.capacity *= 2;
            playersList.players = (Player *)realloc(playersList.players, playersList.capacity * sizeof(Player));
            if (playersList.players == NULL) {
                perror("Memory reallocation failed");
                exit(errno);
            }
        }
    }

    return 0;
}

// Function to handle client requests
void *handle_client(void *arg) {
    Player *player = (Player *)arg;
    char buffer[1024] = {0};
    int valread;

    // Read incoming message from client
    while ((valread = read(player->fd, buffer, sizeof(buffer))) > 0) {
        printf("Client (%s): %s", player->name, buffer);

        // Echo back to client
        if (write(player->fd, buffer, strlen(buffer)) != strlen(buffer)) {
            perror("Write failed");
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }

    printf("Client (%s) disconnected\n", player->name);
    close(player->fd);

    // Cleanup thread resources
    pthread_exit(NULL);
}
