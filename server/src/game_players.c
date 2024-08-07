#include "game_players.h"
#include "colors.h"

void initializeScoresList(ScoresList* list) {
    if (list->players != NULL) free(list->players);

    list->players = NULL;
    list->size = 0;
}

void addPlayerScore(ScoresList* list, char* nickname, int score) {
    if (list == NULL) return;

    // Allocate memory for the new player list
    PlayerScore* newPlayers = realloc(list->players, (list->size + 1) * sizeof(PlayerScore));
    if (newPlayers == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return;
    }

    // Assign the new memory to the list
    list->players = newPlayers;

    // Add the new player to the list
    strncpy(list->players[list->size].name, nickname, MAX_NICKNAME_LENGTH - 1);
    list->players[list->size].name[MAX_NICKNAME_LENGTH - 1] = '\0'; // Ensure null termination
    list->players[list->size].score = score;
    list->size++;
}

int compareScores(const void* a, const void* b) {
    int scoreA = ((PlayerScore*)a)->score;
    int scoreB = ((PlayerScore*)b)->score;
    return scoreB - scoreA;
}

char* createCsvRanks(ScoresList* list) {
    // Sort the list using qsort
    qsort(list->players, list->size, sizeof(PlayerScore), compareScores);
    
    // Calculate the required length for the CSV string
    size_t csvLength = 0;
    for (int i = 0; i < list->size; ++i) {
        csvLength += strlen(list->players[i].name) + 12; // 12 to account for commas and score (maximum 10 digits) and null terminator
    }

    // Allocate memory for the CSV string
    char* csv;
    ALLOCATE_MEMORY(csv, csvLength, "Failed to allocate memory for CSV\n");
    
    // Build the CSV string
    csv[0] = '\0'; // Initialize as an empty string
    for (int i = 0; i < list->size; ++i) {
        char entry[MAX_NICKNAME_LENGTH + 12]; // Buffer for each entry
        snprintf(entry, sizeof(entry), "%s,%d,", list->players[i].name, list->players[i].score);
        strcat(csv, entry);
    }

    return csv;
}

void destroyScoresList(ScoresList* list) {
    free(list->players);
    list->players = NULL;
    list->size = 0;
}


Player* createPlayer(int fd) {
    // Allocate memory for a new Player
    Player* player;
    ALLOCATE_MEMORY(player, sizeof(Player), "Failed to allocate memory for a new player struct");

    // Initialize the Player fields
    player->fd = fd;
    player->tid = 0;
    player->scorerTid = 0;
    player->name[0] = '\0';
    player->score = 0;
    player->wordsCount = 0;
    player->wordsCapacity = 10;

    // Allocate memory for words array
    player->words = (char**)malloc(player->wordsCapacity * sizeof(char*));
    if (player->words == NULL) {
        perror("Failed to allocate memory for player's words array");
        free(player);
        return NULL;
    }

    return player;
}

PlayerList* createPlayerList(void) {
    PlayerList* list;
    ALLOCATE_MEMORY(list, sizeof(PlayerList), "Unable to allocate memory for players list");

    list->players = malloc(INITIAL_CAPACITY * sizeof(Player*));
    if (list->players == NULL) {
        perror("Failed to allocate memory for players array");
        free(list);
        exit(EXIT_FAILURE);
    }

    list->size = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

Player* addPlayer(PlayerList* list, Player* player) {
    // Check if the list needs to be resized
    // We don't care about checking if the size has reached the MAX_CLIENT limit, because the server won't let connect more clients than MAX_CLIENT, creating a bottleneck
    if (list->size == list->capacity) {
        // Double the capacity if it's not zero, otherwise set it to an initial value
        int newCapacity = list->capacity + 8;
        Player** newPlayers = (Player**)realloc(list->players, newCapacity * sizeof(Player*));
        CHECK_NULL(newPlayers, "Failed to reallocate memory for players list");
        list->players = newPlayers;
        list->capacity = newCapacity;
    }

    // Add the new player to the list
    list->players[list->size] = player;
    list->size++;

    // Return a reference to the newly added player
    return player;
}

void removePlayer(PlayerList* list, Player* player) {
    int index = -1;

    // Find the index of the player to be removed
    for (int i = 0; i < list->size; i++) {
        if (list->players[i]->fd == player->fd) {
            index = i;
            break;
        }
    }

    // If the player is not found, return
    if (index == -1) {
        printf("Player not found\n");
        return;
    }

    // Setting the fd to -1 so that the scoresHandler thread knows the player has quitted
    // Not freeing the player allows the reference to still point to an existing piece of memory
    player->fd = -1;
    
    // Shift the remaining players to fill the gap
    for (int i = index; i < list->size - 1; i++) {
        list->players[i] = list->players[i + 1];
    }

    // Decrease the size of the list
    list->size--;
}

void updatePlayerScore(PlayerList* list, int playerFd, int newScore) {
    // Find the player with the given playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i]->fd == playerFd) {
            // Update the player's score
            list->players[i]->score = newScore;
            return;
        }
    }

    printf("Player with ID %d not found\n", playerFd);
}

int getPlayerScore(PlayerList* list, int playerFd) {
    // Find the player with the given playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i]->fd == playerFd) {
            return list->players[i]->score;
        }
    }

    printf("Player with ID %d not found\n", playerFd);
    return -1;
}

int isPlayerAlreadyRegistered(PlayerList* list, int playerFd) {
	for (int i = 0; i < list->size; i++) {
        if (list->players[i]->fd == playerFd) return 1;
    }
    return 0;
}

int isNicknameValid(PlayerList* list, char* nickname) {
    // Firstly check if nickname length is valid
    if (strlen(nickname) > MAX_NICKNAME_LENGTH) return 0;

    // Then check if 
    for (int i = 0; i < list->size; i++) {
        printf("Comparing: %s  =  %s\n", list->players[i]->name, nickname);
        if (strcmp(list->players[i]->name, nickname) == 0) return 0;
    }

    // Then if content is valid (allowed letters and numbers only)
    while (*nickname) {
        char c = *nickname;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return 0;
        nickname++;
    }

    return 1;
}

int didUserAlreadyGuessedWord(PlayerList* list, int playerFd, char* word) {
    for (int i = 0; i < list->size; i++) {
        if (list->players[i]->fd == playerFd) {
            for (int j = 0; j < list->players[i]->wordsCount; j++) {
                if (strcmp(list->players[i]->words[j], word) == 0) return 1;
            }
            break;
        }
    }
    return 0;
}

void addWordToPlayer(PlayerList* list, int playerFd, char* word) {
    // Find the player with the specified playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i]->fd == playerFd) {
            Player* player = list->players[i];
            
            // Check if we need to reallocate memory for the words array
            if (player->wordsCount == player->wordsCapacity) {
                player->wordsCapacity += 10;
                player->words = (char**)realloc(player->words, player->wordsCapacity * sizeof(char*));
                if (player->words == NULL) {
                    perror("Failed to reallocate memory for words array");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Add the new word
            SYSCN(player->words[player->wordsCount], strdup(word), "Failed to duplicate word");
            player->wordsCount++;
            return;
        }
    }
    
    // If the player is not found, handle the error appropriately
    fprintf(stderr, "Player with fd %d not found\n", playerFd);
}

void cleanPlayersListOfWords(PlayerList* list) {
    // Find the player with the specified playerFd
    for (int i = 0; i < list->size; i++) {
        if (list->players[i]->words) {
            for (int j = 0; j < list->players[i]->wordsCount; ++j) free(list->players[i]->words[j]);
            free(list->players[i]->words);
            list->players[i]->wordsCount = 0;
            list->players[i]->wordsCapacity = 10; // Back to initial capacity
            ALLOCATE_MEMORY(list->players[i]->words, list->players[i]->wordsCapacity * sizeof(char*), "Unable to allocate memory for player list of words");
        }
    }
}

void freePlayerList(PlayerList* list) {
    free(list->players);
    free(list);
}

void printPlayerList(PlayerList* list) {
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    for (int i = 0; i < list->size; i++) {
        Player* player = list->players[i];
        printf(YELLOW("%i - %p - tid: %lu fd: %d nick: %s score: %d\n"), i, player, (unsigned long)player->tid, player->fd, player->name, player->score);
        
        printf("  Words:\n");
        for (int j = 0; j < player->wordsCount; j++) {
            printf("    %s\n", player->words[j]);
        }
    }
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
}
