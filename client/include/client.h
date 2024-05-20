#ifndef CLIENT_H
#define CLIENT_H

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
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/select.h>
#include <time.h>
#include "macros.h"
#include "game_matrix.h"

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

// Structure to hold thread arguments
typedef struct {
    int clientFd;
    char* clientInput;
    char* serviceReturnMsg;
    Cell (*matrix)[MATRIX_SIZE];
    int* score;
    int* timeLeft;
} ThreadParams;

/*
Main client loop function.
Starts a loop which simulates a shell through which the client can communicate with the server.
*/
void client(int port);
// void handleReceivedMessage(int clientFd, Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* serviceReturnMsg, int* score, int* timeLeft);
// void sendMessage(char* userInput, int clientFd, char* serviceReturnMsg);
int serializeMessage(const Message* msg, char* buffer);
Message parseMessage(char* buffer);

#endif /* CLIENT_H */