#include "server.h"

volatile ServerState currentState = WAITING_STATE;
PlayerList* playersList;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t list_cond = PTHREAD_COND_INITIALIZER;

void toLowercase(char *str) {
    if (str == NULL) return;

    while (*str != '\0') {
        if (*str >= 'A' && *str <= 'Z') *str = *str + ('a' - 'A');
        str++;
    }
}

int comparePlayers(const void *a, const void *b) {
    Player *playerA = (Player *)a;
    Player *playerB = (Player *)b;
    return playerB->score - playerA->score; // Sort in descending order
}

void *scorerThread() {
    // Allocate memory for the result string
    char result[1024] = "";

    // Lock the mutex to ensure thread safety while accessing playersList
    pthread_mutex_lock(&list_mutex);

    // Sort playersList by score in descending order
    qsort(playersList->players, playersList->size, sizeof(Player), comparePlayers);

    // Build the CSV message
    for (int i = 0; i < playersList->size; i++) {
        char entry[100];
        sprintf(entry, "%s,%d,", playersList->players[i].name, playersList->players[i].score);
        strcat(result, entry);
    }

    // Unlock the mutex after accessing playersList
    pthread_mutex_unlock(&list_mutex);

    // Replace the last comma with a null terminator
    result[strlen(result) - 1] = '\0';

    printf("Final results: %s\n", result);

    // Prepare the response message
    Message responseMsg;
    responseMsg.payload = (char *)malloc(1024);
    responseMsg.type = MSG_PUNTI_FINALI;
    strcpy(responseMsg.payload, result);
    responseMsg.size = strlen(responseMsg.payload);

    // Send the response message to each client
    for (int i = 0; i < playersList->size; i++) {
        sendMessageToClient(responseMsg, playersList->players[i].fd);
    }

    // Free the allocated memory for the response message payload
    free(responseMsg.payload);

    return NULL;
}

void switchState(int signum) {
    if (currentState == WAITING_STATE) {
        currentState = GAME_STATE;
        printf(GREEN("Switched to GAME_STATE\n"));
    } else {
        currentState = WAITING_STATE;
        printf(YELLOW("Switched to WAITING_STATE\n"));

        // Game has just ended, creating csv game ranking to send to the clients
        // Let's start a new "scorer" thread which creates the csv ranking from the players list
        // This new thread will also tell all the client threads to communicate the csv ranking to each client
        pthread_t scorer;
        pthread_create(&scorer, NULL, scorerThread, NULL);
    }

    // Set the next alarm for 1 minute
    alarm(15);
}

unsigned int getRemainingTime() {
    unsigned int remainingTime = alarm(0); // Cancel the current alarm and get the remaining time
    alarm(remainingTime); // Reset the alarm with the remaining time
    return remainingTime;
}

int serializeMessage(const Message* msg, char* buffer) {
    int totalSize = sizeof(char) + sizeof(int) + msg->size;

    // Copy 'type' field into the buffer
    buffer[0] = msg->type;

    // Copy 'size' field into the buffer
    memcpy(buffer + sizeof(char), &(msg->size), sizeof(int));

    // Copy 'payload' into the buffer
    memcpy(buffer + sizeof(char) + sizeof(int), msg->payload, msg->size);

    // Return the total size of the serialized message
    return totalSize;
}

Message parseMessage(char* buffer) {
    // Message structure
    Message msg;

    // Parsing of message type
    msg.type = buffer[0];

    // Parsing of message size
    memcpy(&msg.size, buffer + sizeof(char), sizeof(int));

    // Payload memory allocation and storing
    msg.payload = (char*)malloc(msg.size);
    if (msg.payload == NULL) {
        perror("Failed to allocate memory for payload");
        exit(EXIT_FAILURE);
    }
    memcpy(msg.payload, buffer + sizeof(char) + sizeof(int), msg.size);
    msg.payload[msg.size] = '\0';

    // Printing parsed message
    printf("%c %d %s\n", msg.type, msg.size, msg.payload);
    for (int i = 0; i < msg.size; i++) printf("%02x ", (unsigned char)msg.payload[i]);
    printf("\n\n");

    return msg;
}

void sendMessageToClient(Message msg, int clientFd) {
    int retvalue;
    // Allocating correct amount of bytes
    int totalSize = sizeof(char) + sizeof(int) + msg.size;
    char* buffer = (char*)malloc(totalSize);

    // Serializing msg into buffer
    buffer[0] = msg.type;
    memcpy(buffer + sizeof(char), &msg.size, sizeof(int));
    memcpy(buffer + sizeof(char) + sizeof(int), msg.payload, msg.size);

    // Check what we're sending
    printf("SENDING %d bytes: \n", totalSize);
    for (int i = 0; i < totalSize; i++) printf("%02x ", (unsigned char)buffer[i]);
    printf("\n");

    // Send the serialized message to the server
    SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");
}

void *handleClient(void *arg) {
    ClientThreadParams *params = (ClientThreadParams *)arg;
    if (params == NULL || params->player == NULL || params->playersList == NULL) {
        fprintf(stderr, "Invalid parameters passed to thread\n");
        pthread_exit(NULL);
    }

    char* buffer = (char*)malloc(1024);
    int valread;

    // Read incoming message from client
    while ((valread = read(params->player->fd, buffer, 1024)) > 0) {
        // printf("BUFFER size %d %d, %s\n", strlen(buffer), valread, buffer);
        printf("From client %d: ", params->player->fd);
        Message msg = parseMessage(buffer); // Also prints the parsed buffer content
        // Allocate memory for the response payload
        Message responseMsg;
        responseMsg.payload = (char*)malloc(1024);

        switch (msg.type) {
            case MSG_REGISTRA_UTENTE:
                if (isPlayerAlreadyRegistered(params->playersList, params->player->fd)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Already registered\n");
                
                    // Sending response to client
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, params->player->fd);

                    free(responseMsg.payload);
                } else if (nicknameAlreadyExists(params->playersList, msg.payload)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Nickname invalid\n");
                    
                    // Sending response to client
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, params->player->fd);

                    free(responseMsg.payload);
                } else {
                    // Adding player to players list
                    responseMsg.type = MSG_OK;
                    strcpy(responseMsg.payload, "Game joined\n");
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, params->player->fd);
                    addPlayer(params->playersList, params->player->fd, pthread_self(), msg.payload, 0);

                    // Sending the game matrix if the game is running
                    if (currentState == GAME_STATE) {
                        responseMsg.type = MSG_MATRICE;
                        memcpy(responseMsg.payload, params->matrix, sizeof(params->matrix));
                        responseMsg.size = strlen(responseMsg.payload);
                        sendMessageToClient(responseMsg, params->player->fd);
                        free(responseMsg.payload);
                    }

                    // Also sending time left to client with a custom response message
                    responseMsg.type = currentState == GAME_STATE ? MSG_TEMPO_PARTITA : MSG_TEMPO_ATTESA;
                    char timeLeftString[10];
                    sprintf(timeLeftString, "%d", getRemainingTime());
                    responseMsg.payload = strdup(timeLeftString);
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, params->player->fd);

                    free(responseMsg.payload);
                }
                responseMsg.type == MSG_OK ? printf(GREEN("%s\n"), responseMsg.payload) : printf(RED("%s\n"), responseMsg.payload);
                printPlayerList(params->playersList);

                break;
            case MSG_MATRICE:
                if (currentState == GAME_STATE) {
                    if (isPlayerAlreadyRegistered(params->playersList, params->player->fd)) {
                        responseMsg.type = MSG_MATRICE;
                        memcpy(responseMsg.payload, params->matrix, sizeof(params->matrix));
                    } else {
                        responseMsg.type = MSG_ERR;
                        strcpy(responseMsg.payload, "You're not registered yet\n");
                    }

                    // Sending response to client
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, params->player->fd);

                    free(responseMsg.payload);
                    sleep(0.5);
                }
                
                // Also sending time left to client
                responseMsg.type = currentState == GAME_STATE ? MSG_TEMPO_PARTITA : MSG_TEMPO_ATTESA;
                char timeLeftString[10];
                sprintf(timeLeftString, "%d\n", getRemainingTime());
                // strcpy(responseMsg.payload, timeLeftString);
                responseMsg.payload = strdup(timeLeftString);
                responseMsg.size = strlen(responseMsg.payload);

                // Sending second message
                sendMessageToClient(responseMsg, params->player->fd);

                free(responseMsg.payload);
                break;
            case MSG_PAROLA:
                // Lower-casing given word for future comparisons
                toLowercase(msg.payload);
                printf("TO LOWERCASE: %s\n", msg.payload);

                // Deciding what to response to client
                if (!isPlayerAlreadyRegistered(params->playersList, params->player->fd)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "You're not registered yet\n");
                } else if (currentState == WAITING_STATE) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Wait for the game to start\n");
                } else if (!doesWordExistInMatrix(params->matrix, msg.payload) || !search(params->trieRoot, msg.payload)) {
                    // If not present in the matrix or in the dictionary
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Invalid word\n");
                } else if (didUserAlreadyWroteWord(params->playersList, params->player->fd, msg.payload)) {
                    responseMsg.type = MSG_PUNTI_PAROLA;
                    strcpy(responseMsg.payload, "0\n");
                } else {
                    responseMsg.type = MSG_PUNTI_PAROLA;

                    int earnedPoints = strlen(msg.payload);
                    printf(GREEN("Word %s exists, +%d\n"), msg.payload, strlen(msg.payload));

                    // Updating Player score
                    int updatedScore = getPlayerScore(params->playersList, params->player->fd) + earnedPoints;
                    updatePlayerScore(params->playersList, params->player->fd, updatedScore);

                    // Updating player list of words
                    addWordToPlayer(params->playersList, params->player->fd, msg.payload);

                    // Sending how much points the given word was as string
                    char pointsString[2];
                    sprintf(pointsString, "%d\n", earnedPoints);
                    strcpy(responseMsg.payload, pointsString);

                    // Printing players list to see updated score
                    printPlayerList(params->playersList);
                }

                // Sending response to client
                responseMsg.size = strlen(responseMsg.payload);
                sendMessageToClient(responseMsg, params->player->fd);

                free(responseMsg.payload);
                break;
            default:
                break;
        }
    }

    printf("Client %s disconnected\n", params->player->name);
    removePlayer(params->playersList, params->player->fd);
    printPlayerList(params->playersList);
    close(params->player->fd);

    // Cleanup thread resources
    pthread_exit(NULL);
}

void server(int serverPort, const char *matrixFilename, int gameDuration, unsigned int rndSeed, const char *newDictionaryFile) {
    // Seed the random number generator with current time
    srand(time(NULL));

    playersList = createPlayerList();
    int server_fd, retvalue, new_client_fd;
    fd_set readfds;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[MATRIX_BYTES + sizeof(int)];

    // Inizializzazione della struttura server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverPort);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 127.0.0.1

    // Creazione del socket
    SYSC(server_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Binding
    SYSC(retvalue, bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella bind");

    // Listen
    SYSC(retvalue, listen(server_fd, MAX_CLIENTS), "nella listen");

    // Creating matrix using random letters
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];
    matrixFilename ? createNextMatrixFromFile(matrix, matrixFilename) : initRandomMatrix(matrix);

    // Printing matrix
    printf("Ready to listen.\nGenerated matrix:\n");
    printMatrix(matrix);

    // Loading dictionary TODO: Do it using another thread, it might take time
    TrieNode *root = createNode();
    loadDictionary(root, "include/dictionary_ita.txt");

    // Set up signal handler and initial alarm
    signal(SIGALRM, switchState);
    alarm(15);

    // Main loop
    while (1) {
        // Accept incoming connection
        client_addr_len = sizeof(client_addr);
        SYSC(new_client_fd, accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len), "Accept failed");

        printf("Client accepted, socket fd is %d\n", new_client_fd);

        ClientThreadParams *params = (ClientThreadParams *)malloc(sizeof(ClientThreadParams));
        if (params == NULL) {
            perror("Failed to allocate memory for client thread parameters");
            exit(EXIT_FAILURE);
        }
        memcpy(params->matrix, matrix, sizeof(matrix));
        params->playersList = playersList;

        Player *newPlayer = (Player *)malloc(sizeof(Player));
        if (newPlayer == NULL) {
            perror("Failed to allocate memory for new player");
            exit(EXIT_FAILURE);
        }
        newPlayer->fd = new_client_fd;
        params->player = newPlayer;
        params->trieRoot = root;

        // Creating a new thread to handle the client
        pthread_create(&newPlayer->tid, NULL, handleClient, (void *)params);
        pthread_create(&newPlayer->tid, NULL, sendFinalResults, (void *)params);
    }

    // Chiusura del socket del server (non raggiungibile)
    SYSC(retvalue, close(server_fd), "nella close");
}
