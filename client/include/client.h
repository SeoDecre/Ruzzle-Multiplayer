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
#include <signal.h>
#include "macros.h"
#include "colors.h"
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
#define MAX_SERVICE_RETURN_MSG_SIZE 256

typedef struct {
    char type;        // 1 byte
    int size;         // 4 byte
    char* payload;    // n byte
} Message;

// Structure to hold thread arguments
typedef struct {
    int clientFd;
    char* clientInput;
    char* serverResponse;
    char* shellInfoMessage;
    char* finalScoreboard;
    int* score;
    int* timeLeft;
    Cell (*matrix)[MATRIX_SIZE];
    pthread_mutex_t mutex;
    volatile sig_atomic_t* connectionClosed;
} ThreadParams;

int serializeMessage(const Message* msg, char** buffer);

Message parseMessage(const char* buffer);

void processCommand(const char* command, const char* content, ThreadParams* params);

void sendMessage(void* arg);

void parseAndMemorizeScoreboard(char* finalScoreboard, char* msgPayload);

void displayGameShell(ThreadParams* params);

void processReceivedMessage(Message* msg, ThreadParams* params);

void* handleReceivedMessage(void* arg);

/*
Main client function
Creates a new thread to continuously listen to the server
Uses the main thread to create a shell-alike terminal for user prompts
*/
void client(char* serverName, int port);

#endif /* CLIENT_H */