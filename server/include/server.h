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

// 128 bytes are enough to store every kind of buffer message sent between client and server
#define MAX_BUFFER_BYTES 128

// 512 bytes are enough to store the entire scoreboard rankings string
#define MAX_RANKS_BUFFER_BYTES 512

#define STD_DICTIONARY_FILENAME "../files/dictionary_ita.txt"

// Message struct client-server protocol
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

// Cleans up resources: freeing memory, closing connections and destroying mutexes
// It's used from the handleSigint "CTRL-C detection" function
void cleanup();

// Detects the CTRL-C server closure detection
void handleSigint();

// Switches the global ServerState
// It allows notify threads that the game has just ended or started
void switchState();

// Remaining time getter
unsigned int getRemainingTime();

// Main thread server function
// Accepts new client connections by assigning a new clientCommunicationThread to them
void server(char* serverName, int serverPort, char* passedMatrixFileName, int gameDuration, unsigned int rndSeed, char *newDictionaryFile);

// Main client thread handler function
// Listens to a specific client request and to respond to it
void* clientCommunicationThread(void* player);

// "Side" client thread scores handler function
// A thread with this function is created just for registered clients
// Detects when the game is ended, so that each player can send its score to the global scoresList list
// and the final scoreboard ranks string to its client after the "scorerThread" thread has listed it
void* playerScoreCollectorThread(void* player);

// Thread function used to create a scoreboard ranks string as soon as each player has finished adding its score to the global scoresList list
// It also notifies each "playerScoreCollectorThread" thread so that each one of them can send the final scoreboard to its client
void* scorerThread();

// Fills up a Message reference fields
// It automatically sets the Message size field
void setResponseMsg(Message* responseMsg, char type, char *payload);

// Sends a string-buffered Message to a client using its fd
void sendMessageToClient(Message msg, int fd);

// Parses a string buffer received from a client into its equivalent Message struct element
void parseMessage(char* buffer, Message* msg);

#endif /* SERVER_H */