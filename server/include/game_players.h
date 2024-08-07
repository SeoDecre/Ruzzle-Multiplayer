#ifndef PLAYERLIST_H
#define PLAYERLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "macros.h"

#define MAX_CLIENTS 32
#define INITIAL_CAPACITY 8
#define MAX_NICKNAME_LENGTH 10

// Struct used to handle a client connection and, eventually, the game data
typedef struct {
    int fd;
    pthread_t tid;
    pthread_t scorerTid;
    char name[MAX_NICKNAME_LENGTH + 1]; // For null terminator
    int score;
    char** words;
    int wordsCount;
    int wordsCapacity;
} Player;

// List of Player pointers
// Memory is initially allocated for a INITIAL_CAPACITY number of players, and eventually extended if needed
typedef struct {
    Player** players;
    int size;
    int capacity;
} PlayerList;

// Struct used to collect a specific player score
typedef struct {
    char name[MAX_NICKNAME_LENGTH + 1];
    int score;
} PlayerScore;

// List of PlayerScore structs
// Used by each "playerScoreCollectorThread" thread to collect the personal score
// Then used by the scorer thread to list the final scoreboard ranks
typedef struct {
    PlayerScore* players;
    int size;
} ScoresList;

// Initializes the given ScoresList, cleaning up any past allocations
// This function is called every time the game ends, so that the clients can put their score into a brand new list
void initializeScoresList(ScoresList* list);

// Adds a PlayerScore struct to the ScoresList
// Since players may register or leave the game while the scorer thread is listing the scoreboards, this function progressively allocates memory for any new added element
void addPlayerScore(ScoresList* list, char* nickname, int score);

// Compares two scores for sorting
int compareScores(const void* a, const void* b);

// Function used to sort the ScoresList by scores in ascending order and creating a CSV string of ranks from the ScoresList
char* createCsvRanks(ScoresList* list);

// Destroys the given ScoresList, freeing any allocated memory
void destroyScoresList(ScoresList* list);


// Creates a new Player with the given file descriptor
Player* createPlayer(int fd);

// Creates a new empty PlayerList, allocating space for a number of INITIAL_CAPACITY players
PlayerList* createPlayerList(void);

// Adds a player to the PlayerList
Player* addPlayer(PlayerList* list, Player* player);

// Removes a player from the PlayerList
// It doesn't free the memory associated with the player, since its fields still may be used by the playerScoreCollectorThread
void removePlayer(PlayerList* list, Player* player);

// Updates the score of a player in the PlayerList
void updatePlayerScore(PlayerList* list, int playerFd, int newScore);

// Retrieves the score of a player from the PlayerList
int getPlayerScore(PlayerList* list, int playerFd);

// Checks if a player with the given file descriptor is already registered
int isPlayerAlreadyRegistered(PlayerList* list, int playerFd);

// Checks if a nickname is valid and not already taken in the PlayerList
int isNicknameValid(PlayerList* list, char* nickname);

// Checks if a player has already guessed a specific word
int didUserAlreadyGuessedWord(PlayerList* list, int playerFd, char* word);

// Adds a word to the list of words associated with a player
void addWordToPlayer(PlayerList* list, int playerFd, char* word);

// Frees the list of words for all players in the PlayerList
void cleanPlayersListOfWords(PlayerList* list);

// Frees the memory allocated for the PlayerList and its elements
void freePlayerList(PlayerList* list);

// Prints details of all players in the PlayerList
void printPlayerList(PlayerList* list);

#endif /* PLAYERLIST_H */
