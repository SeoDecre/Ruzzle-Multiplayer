#ifndef PLAYERLIST_H
#define PLAYERLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CLIENTS 32
#define INITIAL_CAPACITY 10
#define MAX_NICKNAME_LENGTH 20

typedef struct {
    int fd;
    pthread_t tid;
    char name[MAX_NICKNAME_LENGTH];
    int score;
    char** words;
    int wordsCount;
    int wordsCapacity;
} Player;

typedef struct {
    Player* players;
    int size;
    int capacity;
} PlayerList;

PlayerList* createPlayerList();
void resizePlayerList(PlayerList* list);
Player* addPlayer(PlayerList* list, int fd, pthread_t tid, char* nickname, int score);
void removePlayer(PlayerList* list, int playerFd);
void updatePlayerScore(PlayerList* list, int playerFd, int newScore);
int getPlayerScore(PlayerList* list, int playerFd);
void updatePlayerNickname(PlayerList* list, int playerFd, char* newNickname);
int isPlayerAlreadyRegistered(PlayerList* list, int playerFd);
int nicknameAlreadyExists(PlayerList* list, char* nickname);
int didUserAlreadyWroteWord(PlayerList* list, int playerFd, char* word);
void addWordToPlayer(PlayerList* list, int playerFd, char* word);
void freePlayerList(PlayerList* list);
void printPlayerList(PlayerList* list);

#endif /* PLAYERLIST_H */
