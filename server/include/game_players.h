#ifndef PLAYERLIST_H
#define PLAYERLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "macros.h"

#define MAX_CLIENTS 2
#define INITIAL_CAPACITY 8
#define MAX_NICKNAME_LENGTH 20

typedef struct {
    int fd;
    pthread_t tid;
    pthread_t scorerTid;
    char name[MAX_NICKNAME_LENGTH];
    int score;
    char** words;
    int wordsCount;
    int wordsCapacity;
} Player;

typedef struct {
    Player** players;
    int size;
    int capacity;
} PlayerList;

typedef struct {
    char name[MAX_NICKNAME_LENGTH];
    int score;
} PlayerScore;

typedef struct {
    PlayerScore* players;
    int size;
} ScoresList;

void initializeScoresList(ScoresList* list);
void addPlayerScore(ScoresList* list, char* nickname, int score);
int compareScores(const void* a, const void* b);
char* createCsvRanks(ScoresList* list);
void destroyScoresList(ScoresList* list);

Player* createPlayer(int fd);
PlayerList* createPlayerList(void);
Player* addPlayer(PlayerList* list, Player* player);
void removePlayer(PlayerList* list, Player* player);
void updatePlayerScore(PlayerList* list, int playerFd, int newScore);
int getPlayerScore(PlayerList* list, int playerFd);
void updatePlayerNickname(PlayerList* list, int playerFd, char* newNickname);
int isPlayerAlreadyRegistered(PlayerList* list, int playerFd);
int nicknameAlreadyExists(PlayerList* list, char* nickname);
int didUserAlreadyWroteWord(PlayerList* list, int playerFd, char* word);
void addWordToPlayer(PlayerList* list, int playerFd, char* word);
void cleanPlayersListOfWords(PlayerList* list);
int comparePlayers(const void *a, const void *b);
void freePlayerList(PlayerList* list);
void printPlayerList(PlayerList* list);

#endif /* PLAYERLIST_H */
