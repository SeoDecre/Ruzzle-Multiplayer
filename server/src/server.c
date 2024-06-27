#include "server.h"

volatile ServerState currentState = WAITING_STATE;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t list_cond = PTHREAD_COND_INITIALIZER;

PlayerList* playersList;
PlayerScoreNode* scoresList;
Cell matrix[MATRIX_SIZE][MATRIX_SIZE];
TrieNode* trieRoot;
int matchTime;

int isGameEnded = 0;
int areScoresInPlayersListReady = 0;
int isCSVResultsScoreboardReady = 0;

void toLowercase(char *str) {
    if (str == NULL) return;

    while (*str != '\0') {
        if (*str >= 'A' && *str <= 'Z') *str = *str + ('a' - 'A');
        str++;
    }
}

void switchState() {
    if (currentState == WAITING_STATE) {
        currentState = GAME_STATE;
        printf(GREEN("Switched to GAME_STATE\n"));
    } else {
        currentState = WAITING_STATE;
        printf(YELLOW("Switched to WAITING_STATE\n"));

        // Initializating the scores list that will be filled by the client threads
        scoresList = createPlayerScoreList(playersList->size);

        // Triggering the scorer thread and the sendFinalResults threads
        isGameEnded = 1;
    }

    // Set the next alarm for n seconds
    alarm(matchTime);
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
    int retValue;
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
    SYSC(retValue, write(clientFd, buffer, totalSize), "nella write");
}

void *scorerThread() {
    // Lock the mutex to ensure thread safety while accessing playersList
    pthread_mutex_lock(&list_mutex);

    // Wait until all threads have added their scores
    while (!areScoresInPlayersListReady) {
        pthread_cond_wait(&list_cond, &list_mutex);
    }

    // Allocate memory for the result string
    char result[1024] = "";

    // Sort scoresList by score in descending order
    qsort(scoresList->players, scoresList->size, sizeof(Player), comparePlayers);

    // Build the CSV message
    for (int i = 0; i < scoresList->size; i++) {
        char entry[100];
        sprintf(entry, "%s,%d,", scoresList->players[i].name, scoresList->players[i].score);
        strcat(result, entry);
    }

    // Unlock the mutex after accessing scoresList
    pthread_mutex_unlock(&list_mutex);

    // Replace the last comma with a null terminator
    result[strlen(result) - 1] = '\0';

    // Storing the results into the shared structure

    printf("Final results: %s\n", result);

    // Resetting areScoresInPlayersListReady variable for next uses
    areScoresInPlayersListReady = 0;

    // Notify each sendFinalResults thread the CSV results scoreboard is ready
    isCSVResultsScoreboardReady = 1;

    // Prepare the response message
    // Message responseMsg;
    // responseMsg.payload = (char *)malloc(1024);
    // responseMsg.type = MSG_PUNTI_FINALI;
    // strcpy(responseMsg.payload, result);
    // responseMsg.size = strlen(responseMsg.payload);

    // Free the allocated memory for the response message payload
    // free(responseMsg.payload);

    return NULL;
}

void* scoresHandler(void* player) {
    pthread_mutex_lock(&list_mutex);
    while (!isGameEnded) {
        pthread_cond_wait(&list_cond, &list_mutex);
    }

    // If game is ended then collect all the scores and put them into the shared scoreboardList structure
    addScorePlayer(scoresList, player->name, player->score);

    // The last thread updating its score player will notify the scorer thread
    if (scoresList->size == playersList->size) {
        areScoresInPlayersListReady = 1;
        isGameEnded = 0;
    }

    pthread_mutex_unlock(&list_mutex);

    // Signal the scorerThread that final results are ready
    pthread_mutex_lock(&list_mutex);
    finalResultsReady = 1;
    pthread_cond_signal(&list_cond);
    pthread_mutex_unlock(&list_mutex);

    return NULL;
}

void* handleClient(void* player) {
    Player* playerPointer = (Player *)player;

    char* buffer = (char*)malloc(1024);
    int valread;

    // Read incoming message from client
    while ((valread = read(playerPointer->fd, buffer, 1024)) > 0) {
        // printf("BUFFER size %d %d, %s\n", strlen(buffer), valread, buffer);
        printf("From client %d: ", playerPointer->fd);
        Message msg = parseMessage(buffer); // Also prints the parsed buffer content
        // Allocate memory for the response payload
        Message responseMsg;
        responseMsg.payload = (char*)malloc(1024);

        switch (msg.type) {
            case MSG_REGISTRA_UTENTE:
                if (isPlayerAlreadyRegistered(playersList, playerPointer->fd)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Already registered\n");
                
                    // Sending response to client
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, playerPointer->fd);

                    free(responseMsg.payload);
                } else if (nicknameAlreadyExists(playersList, msg.payload)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Nickname invalid\n");
                    
                    // Sending response to client
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, playerPointer->fd);

                    free(responseMsg.payload);
                } else {
                    // Adding player to players list
                    responseMsg.type = MSG_OK;
                    strcpy(responseMsg.payload, "Game joined\n");
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, playerPointer->fd);
                    addPlayer(playersList, playerPointer->fd, pthread_self(), msg.payload, 0);

                    // Sending the game matrix if the game is running
                    if (currentState == GAME_STATE) {
                        responseMsg.type = MSG_MATRICE;
                        memcpy(responseMsg.payload, matrix, sizeof(matrix));
                        responseMsg.size = strlen(responseMsg.payload);
                        sendMessageToClient(responseMsg, playerPointer->fd);
                        free(responseMsg.payload);
                    }

                    // Also sending time left to client with a custom response message
                    responseMsg.type = currentState == GAME_STATE ? MSG_TEMPO_PARTITA : MSG_TEMPO_ATTESA;
                    char timeLeftString[10];
                    sprintf(timeLeftString, "%d", getRemainingTime());
                    responseMsg.payload = strdup(timeLeftString);
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, playerPointer->fd);

                    free(responseMsg.payload);
                }
                responseMsg.type == MSG_OK ? printf(GREEN("%s\n"), responseMsg.payload) : printf(RED("%s\n"), responseMsg.payload);
                printPlayerList(playersList);

                break;
            case MSG_MATRICE:
                if (currentState == GAME_STATE) {
                    if (isPlayerAlreadyRegistered(playersList, playerPointer->fd)) {
                        responseMsg.type = MSG_MATRICE;
                        memcpy(responseMsg.payload, matrix, sizeof(matrix));
                    } else {
                        responseMsg.type = MSG_ERR;
                        strcpy(responseMsg.payload, "You're not registered yet\n");
                    }

                    // Sending response to client
                    responseMsg.size = strlen(responseMsg.payload);
                    sendMessageToClient(responseMsg, playerPointer->fd);

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
                sendMessageToClient(responseMsg, playerPointer->fd);

                free(responseMsg.payload);
                break;
            case MSG_PAROLA:
                // Lower-casing given word for future comparisons
                toLowercase(msg.payload);
                printf("TO LOWERCASE: %s\n", msg.payload);

                // Deciding what to response to client
                if (!isPlayerAlreadyRegistered(playersList, playerPointer->fd)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "You're not registered yet");
                } else if (currentState == WAITING_STATE) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Waiting for match to start");
                } else if (!doesWordExistInMatrix(matrix, msg.payload) || !search(trieRoot, msg.payload)) {
                    // If not present in the matrix or in the dictionary
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Invalid word");
                } else if (didUserAlreadyWroteWord(playersList, playerPointer->fd, msg.payload)) {
                    responseMsg.type = MSG_PUNTI_PAROLA;
                    strcpy(responseMsg.payload, "0\n");
                } else {
                    responseMsg.type = MSG_PUNTI_PAROLA;

                    int earnedPoints = strlen(msg.payload);
                    printf(GREEN("Word %s exists, +%d\n"), msg.payload, (int)strlen(msg.payload));

                    // Updating Player score
                    int updatedScore = getPlayerScore(playersList, playerPointer->fd) + earnedPoints;
                    updatePlayerScore(playersList, playerPointer->fd, updatedScore);

                    // Updating player list of words
                    addWordToPlayer(playersList, playerPointer->fd, msg.payload);

                    // Sending how much points the given word was as string
                    char pointsString[2];
                    sprintf(pointsString, "%d", earnedPoints);
                    strcpy(responseMsg.payload, pointsString);

                    // Printing players list to see updated score
                    printPlayerList(playersList);
                }

                // Sending response to client
                responseMsg.size = strlen(responseMsg.payload);
                sendMessageToClient(responseMsg, playerPointer->fd);

                free(responseMsg.payload);
                break;
            default:
                break;
        }
    }

    printf("Client %s disconnected\n", playerPointer->name);
    removePlayer(playersList, playerPointer->fd);
    printPlayerList(playersList);
    close(playerPointer->fd);

    // Cleanup thread resources
    pthread_exit(NULL);
}

void server(char* serverName, int serverPort, char* matrixFilename, int gameDuration, unsigned int rndSeed, char *newDictionaryFile) {
    playersList = createPlayerList();
    int serverFd, retValue, newClientFd;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength;
    srand(rndSeed);

    // Inizializzazione della struttura serverAddress
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    CHECK_NEGATIVE(inet_pton(AF_INET, serverName, &serverAddress.sin_addr), "Invalid address");

    // Creazione del socket, binding e listen
    SYSC(serverFd, socket(AF_INET, SOCK_STREAM, 0), "Unable to create socket socket");
    SYSC(retValue, bind(serverFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)), "Enable to bind socket");
    SYSC(retValue, listen(serverFd, MAX_CLIENTS), "Unable to start listen");

    // Filling matrix up
    matrixFilename ? createNextMatrixFromFile(matrix, matrixFilename) : initRandomMatrix(matrix);
    printf("Ready to listen.\nGenerated matrix:\n");
    printMatrix(matrix);

    // Loading dictionary TODO: Do it using another thread, it might take time
    trieRoot = createNode();
    loadDictionary(trieRoot, newDictionaryFile ? newDictionaryFile : "include/dictionary_ita.txt");

    // Setting up signal handler and initial alarm
    signal(SIGALRM, switchState);
    matchTime = gameDuration;
    alarm(gameDuration);

    // Let's start a new scorer thread which creates the csv ranking from the players list
    // This new thread will also tell all the client threads to communicate the csv ranking to each client
    pthread_t scorer;
    pthread_create(&scorer, NULL, scorerThread, NULL);

    // Main loop
    while (1) {
        // Accept incoming connection
        clientAddressLength = sizeof(clientAddress);
        SYSC(newClientFd, accept(serverFd, (struct sockaddr *)&clientAddress, &clientAddressLength), "Failed accepting a new client");
        printf("Client accepted, socket fd is %d\n", newClientFd);

        Player* newPlayer;
        ALLOCATE_MEMORY(newPlayer, sizeof(Player), "Failed to allocate memory for a new player struct");
        newPlayer->fd = newClientFd;

        // Creating a new thread to handle the client requestes and responses
        pthread_create(&newPlayer->tid, NULL, handleClient, (void *)newPlayer);

        // Creating a new thread to handle the scores collection system and sending the csv scoreboard
        pthread_create(&newPlayer->tid, NULL, scoresHandler, (void *)newPlayer);
    }

    // Socket server closure
    SYSC(retValue, close(serverFd), "Failed closing the socket");
}
