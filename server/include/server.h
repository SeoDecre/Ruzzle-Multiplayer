#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>
#include "macros.h"
#include "game_matrix.h"
#include "game_players.h"
#include "dictionary.h"
#include "colors.h"

#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

typedef struct {
    char type;        // 1 byte
    int size;         // 4 byte
    char* payload;    // n byte
} Message;

typedef struct {
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];
    Player* player;
    TrieNode* trieRoot;
} ClientThreadParams;

typedef enum {
    WAITING_STATE,
    GAME_STATE
} ServerState;

void server(char* serverName, int serverPort, char* matrixFilename, int gameDuration, unsigned int rndSeed, char *newDictionaryFile);
void* handleClient(void* player);
Message parseMessage(char* buffer);
int serializeMessage(const Message* msg, char* buffer);
void sendMessageToClient(Message msg, int fd);

#endif /* SERVER_H */