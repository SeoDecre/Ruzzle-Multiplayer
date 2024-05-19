#include "game_players.h"
#define RED(string) "\x1b[31m" string "\x1b[0m"
#define GREEN(string) "\x1b[32m" string "\x1b[0m"
#define YELLOW(string) "\x1b[33m" string "\x1b[0m"
#define BLUE(string) "\x1b[34m" string "\x1b[0m"
#define MAGENTA(string) "\x1b[35m" string "\x1b[0m"
#define CYAN(string) "\x1b[36m" string "\x1b[0m"

PlayerList* createPlayerList() {
    PlayerList* list = malloc(sizeof(PlayerList));

    if (list == NULL) {
        perror("Failed to allocate memory for PlayerList");
        exit(EXIT_FAILURE);
    }

    list->players = malloc(INITIAL_CAPACITY * sizeof(Player));

    if (list->players == NULL) {
        perror("Failed to allocate memory for players array");
        free(list);
        exit(EXIT_FAILURE);
    }

    list->size = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

void resizePlayerList(PlayerList* list) {
    int newCapacity = list->capacity * 2;
    Player *newPlayers = realloc(list->players, newCapacity * sizeof(Player));
    if (newPlayers == NULL) {
        perror("Failed to reallocate memory for players array");
        exit(EXIT_FAILURE);
    }
    list->players = newPlayers;
    list->capacity = newCapacity;
}

Player* addPlayer(PlayerList* list, int fd, pthread_t tid, char* nickname, int score) {
    if (list == NULL) {
        fprintf(stderr, "PlayerList is NULL\n");
        return NULL;
    }

    if (list->size >= list->capacity) resizePlayerList(list);

    // Allocate memory for the new player
    Player *newPlayer = (Player *)malloc(sizeof(Player));
    if (newPlayer == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the new player
    newPlayer->fd = fd;
    newPlayer->tid = tid;
    strcpy(newPlayer->name, nickname);
    newPlayer->score = score;

    // Add the new player to the list
    list->players[list->size++] = *newPlayer;

    // Free the allocated memory for newPlayer (not needed after copying)
    return newPlayer;
}

void removePlayer(PlayerList* list, int playerFd) {
    // Find the index of the player with the given playerFd
    int index = -1;
    for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("Player with ID %d not found\n", playerFd);
        return;
    }

    // Shift remaining players to overwrite the removed player
    for (int i = index; i < list->size - 1; i++) {
        list->players[i] = list->players[i + 1];
    }

    // Decrement the size of the list
    list->size--;
}

void updatePlayerScore(PlayerList* list, int playerFd, int newScore) {
    // Find the player with the given playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) {
            // Update the player's score
            list->players[i].score = newScore;
            return;
        }
    }

    printf("Player with ID %d not found\n", playerFd);
}

int getPlayerScore(PlayerList* list, int playerFd) {
    // Find the player with the given playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) {
            return list->players[i].score;
        }
    }

    printf("Player with ID %d not found\n", playerFd);
}

void updatePlayerNickname(PlayerList* list, int playerFd, char* newNickname) {
    // Find the player with the given playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) {
            // Update the player's nickname
            strcpy(list->players[i].name, newNickname);
            return;
        }
    }

    printf("Player with ID %d not found\n", playerFd);
}

int isPlayerAlreadyRegistered(PlayerList* list, int playerFd) {
	for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) return 1;
    }
    return 0;
}

int nicknameAlreadyExists(PlayerList* list, char* nickname) {
    for (int i = 0; i < list->size; i++) {
        if (strcmp(list->players[i].name, nickname) == 0) return 1;
    }
    return 0;
}

void freePlayerList(PlayerList* list) {
    free(list->players);
    free(list);
}

void printPlayerList(PlayerList* list) {
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    for (int i = 0; i < list->size; i++) {
        printf(YELLOW("%i - tid: %lu fd: %d nick: %s score: %d\n"), i, (unsigned long)list->players[i].tid, list->players[i].fd, list->players[i].name, list->players[i].score);
    }
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
}
