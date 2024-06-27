#include "game_players.h"
#include "colors.h"

ScoresList* createPlayerScoreList(int length) {
    ScoresList* list = malloc(sizeof(PlayerScore));

    list->players = malloc(length * sizeof(PlayerScore));
    list->size = 0;

    return list;
}

PlayerScore* addPlayerScore(ScoresList* list, char* nickname, int score) {
    // Allocate memory for the new player
    Player *newPlayer = (Player *)malloc(sizeof(Player));

    // Initialize the new player
    strcpy(newPlayer->name, nickname);
    newPlayer->score = score;

    // Add the new player to the list
    list->players[list->size++] = *newPlayer;

    // Free the allocated memory for newPlayer (not needed after copying)
    return newPlayer;
}

void freePlayerList(ScoresList* list) {
    free(list->players);
    free(list);
}


PlayerList* createPlayerList(void) {
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
    newPlayer->wordsCount = 0;
    newPlayer->wordsCapacity = 10;  // Initial capacity
    newPlayer->words = (char**)malloc(newPlayer->wordsCapacity * sizeof(char*));

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
    return -1;
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

int didUserAlreadyWroteWord(PlayerList* list, int playerFd, char* word) {
    for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) {
            for (int j = 0; j < list->players[i].wordsCount; j++) {
                if (strcmp(list->players[i].words[j], word) == 0) {
                    return 1;
                }
            }
            break;
        }
    }
    return 0;
}

void addWordToPlayer(PlayerList* list, int playerFd, char* word) {
    // Find the player with the specified playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i].fd == playerFd) {
            Player* player = &list->players[i];
            
            // Check if we need to reallocate memory for the words array
            if (player->wordsCount == player->wordsCapacity) {
                player->wordsCapacity = (player->wordsCapacity > 0) ? player->wordsCapacity * 2 : 10;
                player->words = (char**)realloc(player->words, player->wordsCapacity * sizeof(char*));
                if (player->words == NULL) {
                    perror("Failed to reallocate memory for words array");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Add the new word
            player->words[player->wordsCount] = strdup(word);
            if (player->words[player->wordsCount] == NULL) {
                perror("Failed to duplicate word");
                exit(EXIT_FAILURE);
            }
            player->wordsCount++;
            return;
        }
    }
    
    // If the player is not found, handle the error appropriately
    fprintf(stderr, "Player with fd %d not found\n", playerFd);
}

int comparePlayers(const void *a, const void *b) {
    Player *playerA = (Player *)a;
    Player *playerB = (Player *)b;
    return playerB->score - playerA->score; // Sort in descending order
}

void freePlayerList(PlayerList* list) {
    free(list->players);
    free(list);
}

void printPlayerList(PlayerList* list) {
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    for (int i = 0; i < list->size; i++) {
        Player* player = &list->players[i];
        printf(YELLOW("%i - tid: %lu fd: %d nick: %s score: %d\n"), i, (unsigned long)player->tid, player->fd, player->name, player->score);
        
        printf("  Words:\n");
        for (int j = 0; j < player->wordsCount; j++) {
            printf("    %s\n", player->words[j]);
        }
    }
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
}
