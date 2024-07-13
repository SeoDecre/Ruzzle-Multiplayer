#include "server.h"

int serverFd;
volatile ServerState currentState = WAITING_STATE;

// pthread_mutex_t playersMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t scoresMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gameEndedCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t scoresListCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t CSVResultsCond = PTHREAD_COND_INITIALIZER;

PlayerList* playersList;
ScoresList scoresList;
Cell matrix[MATRIX_SIZE][MATRIX_SIZE];
TrieNode* trieRoot;
int connectedClients;
int matchTime;
char* csvResult;
char* matrixFileName;

int isGameEnded;
int isScoresListReady;
int isCSVResultsScoreboardReady;

void cleanup() {
    // Socket server closure
    int retValue;
    SYSC(retValue, close(serverFd), "Failed closing the server socket");

    // Removing all living threads
    for (int i = 0; i < playersList->size; i++) removePlayer(playersList, playersList->players[i]);
    isGameEnded = 1;
    pthread_cond_broadcast(&gameEndedCond);

    // Destroying mutex and conditions
    pthread_mutex_destroy(&scoresMutex);
    pthread_cond_destroy(&gameEndedCond);
    pthread_cond_destroy(&CSVResultsCond);

    // Cleaning resources
    destroyScoresList(&scoresList);
    freePlayerList(playersList);
    if (csvResult) free(csvResult);
    if (matrixFileName) free(matrixFileName);

    printf(RED("SERVER ENDED\n"));
}

void handleSigint() {
    cleanup();
    exit(0);
}

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

        // Resetting for next uses
        isGameEnded = 0;
        isScoresListReady = 0;
        isCSVResultsScoreboardReady = 0;

        // Preparing new matrix for next game
        printf("FILE NAME: %s\n", matrixFileName);
        matrixFileName ? createNextMatrixFromFile(matrix, matrixFileName) : initRandomMatrix(matrix);
        printf("Generated matrix:\n");
        printMatrix(matrix);
    } else {
        currentState = WAITING_STATE;
        printf(YELLOW("Switched to WAITING_STATE\n"));

        // Initializating the scores list that will be filled by the client threads with their scores
        // We don't care if the playersList is empty, the scoresList will be initialized anyway
        // The scoresList memory allocation increases dynamically, adapting to possible registration in the meanwhile
        initializeScoresList(&scoresList);

        // Triggering the sendFinalResults threads
        // Even though there might not be registered players, we trigger the gameEndedCond so that aventual zombie scoresHandler threads can be canceled
        isGameEnded = 1;
        pthread_cond_broadcast(&gameEndedCond);
    }

    // Set the next alarm for n seconds
    currentState == GAME_STATE ? alarm(matchTime) : alarm(10);
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
    // for (int i = 0; i < msg.size; i++) printf("%02x ", (unsigned char)msg.payload[i]);
    // printf("\n\n");

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
    printf("Sending: %c %d %s\n", msg.type, msg.size, msg.payload);
    // printf("SENDING %d bytes: \n", totalSize);
    // for (int i = 0; i < totalSize; i++) printf("%02x ", (unsigned char)buffer[i]);
    // printf("\n");

    // Send the serialized message to the server
    SYSC(retValue, write(clientFd, buffer, totalSize), "nella write");
}

void* scorerThread() {
    while (1) {
        // Lock the mutex to ensure thread safety while accessing scoresList
        // It could possibly be done without it, because the scorer thread acess the scoresList only after it has been filled up
        pthread_mutex_lock(&scoresMutex);

        // Wait until all threads have added their scores
        while (!isScoresListReady) {
            pthread_cond_wait(&scoresListCond, &scoresMutex);
        }

        printf(GREEN("All players added their score\n"));

        // Resetting for next uses
        isGameEnded = 0;
        isScoresListReady = 0;

        // Creating csv ranks result
        if (csvResult) free(csvResult); // Cleaning from past uses
        csvResult = createCsvRanks(&scoresList);

        // Unlock the mutex after accessing scoresList
        pthread_mutex_unlock(&scoresMutex);

        printf("Final results have been listed: %s\n", csvResult);

        // Notify each sendFinalResults thread the CSV results scoreboard is ready
        isCSVResultsScoreboardReady = 1;
        pthread_cond_broadcast(&CSVResultsCond);
    }

    return NULL;
}

void* scoresHandler(void* player) {
    Player* playerPointer = (Player *)player;
    printf("PLAYER: %s - %d - score %d\n", playerPointer->name, playerPointer->fd, playerPointer->score);

    // We don't have to worry accessing playerPointer without calling the playersMutex because the handleClient threads use the scoresMutex to prevent race conditions
    while (1) {
        pthread_mutex_lock(&scoresMutex);
        
        while (!isGameEnded) {
            printf("%s is waiting for game to end...\n", playerPointer->name);

            // Now the scoresMutex gets unlocked and the thread waits till the gameEndedCond gets triggered by the main thread
            pthread_cond_wait(&gameEndedCond, &scoresMutex);
        }

        // Here the scoresMutex is locked, so that we can make safely access the scoresList shared list

        // Since this thread could be alive even though the related player has quitted, we check if that's the case and use this safe spot to clean the resources up
        if (playerPointer->fd < 0) {
            printf(RED("%s has quitted previously, scoresHandler thread ended\n"), playerPointer->name);
    
            // Free the memory associated with the player
            free(playerPointer->words);
            free(playerPointer);
    
            // Unlocking the mutex
            pthread_mutex_unlock(&scoresMutex);
    
            pthread_exit(NULL);
        }

        // If game is ended then collect all the scores and put them into the shared scoreboardList structure
        // This client-dedicated thread adds its own score in the list
        addPlayerScore(&scoresList, playerPointer->name, playerPointer->score);
        printf("%s adds %d\n", playerPointer->name, playerPointer->score);

        // The last thread updating its score player will notify the scorer thread
        if (scoresList.size >= playersList->size) {
            printf(MAGENTA("%s was the last, scores done\n"), playerPointer->name);
            // Notifies the scorer thread saying the scores list is ready
            isScoresListReady = 1;
            
            // Now the scoresMutex gets unlocked again and the thread waits till the scoresList gets triggered by the scorer thread
            pthread_cond_signal(&scoresListCond);
        }

        // Then it waits for the scorer thread to create the final csv results string
        while(!isCSVResultsScoreboardReady) {
            printf("%s is waiting for scorer thread to make ranks...\n", playerPointer->name);
            pthread_cond_wait(&CSVResultsCond, &scoresMutex);
        }
        
        // Prepare the response message to send to this specific client
        printf(GREEN("Sending rank results to %s\n"), playerPointer->name);
        Message responseMsg;
        ALLOCATE_MEMORY(responseMsg.payload, 512, "Unable to allocate memory for scoreboard message");
        responseMsg.type = MSG_PUNTI_FINALI;
        strcpy(responseMsg.payload, csvResult);
        responseMsg.size = strlen(responseMsg.payload);
        sendMessageToClient(responseMsg, playerPointer->fd);

        free(responseMsg.payload);

        pthread_mutex_unlock(&scoresMutex);
    }

    printf(RED("Thread ended\n"));

    // Free the memory associated with the player
    free(playerPointer->words);
    free(playerPointer);
    
    pthread_exit(NULL);
}

void* handleClient(void* player) {
    Player* playerPointer = (Player *)player;

    char* buffer = (char*)malloc(1024);
    int valread;

    // Updating connected clients counter
    pthread_mutex_lock(&scoresMutex);
    connectedClients++;
    printf(YELLOW("%d CLIENTS\n"), connectedClients);
    pthread_mutex_unlock(&scoresMutex);

    // Read incoming message from client
    while ((valread = read(playerPointer->fd, buffer, 1024)) > 0) {
        // printf("BUFFER size %d %d, %s\n", strlen(buffer), valread, buffer);
        printf("From client %d: ", playerPointer->fd);
        Message msg = parseMessage(buffer); // Also prints the parsed buffer content
        // Allocate memory for the response payload
        Message responseMsg;
        responseMsg.payload = (char*)malloc(1024);

        // pthread_mutex_lock(&playersMutex);
        pthread_mutex_lock(&scoresMutex);

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
                    // Player* newPlayer = 
                    playerPointer->tid = pthread_self();
                    strcpy(playerPointer->name, msg.payload);
                    addPlayer(playersList, playerPointer);

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

                    // Creating a new thread to handle the scores collection system and sending the csv scoreboard
                    // This new player-specific thread is created just for registered players
                    // So that only the client playing the game will be in the scoreboard
                    pthread_create(&playerPointer->scorerTid, NULL, scoresHandler, (void *)playerPointer);

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
                responseMsg.payload = strdup(timeLeftString);
                responseMsg.size = strlen(responseMsg.payload);

                // Sending second message
                sendMessageToClient(responseMsg, playerPointer->fd);

                free(responseMsg.payload);
                break;
            case MSG_PAROLA:
                // Lower-casing given word for future comparisons
                toLowercase(msg.payload);

                // Replacing "Qu" with 'q' so that both the functions used to search the word in the matrix or in the trie don't have to
                replaceQu(msg.payload);

                printf("EDITED WORD: %s\n", msg.payload);

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

                    // Updating player list of sent words
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

        // pthread_mutex_unlock(&playersMutex);
        pthread_mutex_unlock(&scoresMutex);
    }

    printf(RED("Client %s disconnected\n"), playerPointer->name);

    // I choosed to don't pthread_delete the scoresHandler thread directly here
    // In general, we don't really want to violently kill a thread, but instead we want to ask it to terminate
    // That way we can be sure that the child is quitting at a safe spot and all its resources are cleaned up
    // In this case, we let it alive, so that it will exit by itself as soon as it realizes the player has quitted
    // pthread_mutex_lock(&playersMutex);
    pthread_mutex_lock(&scoresMutex);
    removePlayer(playersList, playerPointer);
    connectedClients--;
    printf(YELLOW("%d CLIENTS\n"), connectedClients);
    // pthread_mutex_unlock(&playersMutex);
    pthread_mutex_unlock(&scoresMutex);

    // Printing players list after disconnection
    printPlayerList(playersList);

    // The player fd must be closed so that the server won't try to communicate with this client in the future
    close(playerPointer->fd);

    // Exit
    pthread_exit(NULL);
}

void server(char* serverName, int serverPort, char* passedMatrixFileName, int gameDuration, unsigned int rndSeed, char *newDictionaryFile) {
    playersList = createPlayerList();

    isGameEnded = 0;
    isScoresListReady = 0;
    isCSVResultsScoreboardReady = 0;
    
    connectedClients = 0;
    
    int retValue, newClientFd;
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
    if (passedMatrixFileName) {
        matrixFileName = (char*)malloc(strlen(passedMatrixFileName) + 1);
        strcpy(matrixFileName, passedMatrixFileName);
    }

    // Loading dictionary
    // It might take time, so a new thread could be used to load it
    // But since we start the server at the WAITING_STATE, the function has plenty of time to be executed
    trieRoot = createNode();
    loadDictionary(trieRoot, newDictionaryFile ? newDictionaryFile : "../files/dictionary_ita.txt");

    // Setting up signal handler and initial alarm
    signal(SIGALRM, switchState);
    matchTime = gameDuration;
    alarm(10);

    // Signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, handleSigint);

    // Let's start a new scorer thread which creates the csv ranking from the players list
    // This new thread will also tell all the client threads to communicate the csv ranking to each client
    pthread_t scorer;
    pthread_create(&scorer, NULL, scorerThread, NULL);

    // Main loop
    while (1) {
        // Accept incoming connection
        clientAddressLength = sizeof(clientAddress);
        SYSC(newClientFd, accept(serverFd, (struct sockaddr *)&clientAddress, &clientAddressLength), "Failed accepting a new client");
        
        pthread_mutex_lock(&scoresMutex);
        if (connectedClients >= MAX_CLIENTS) {
            Message responseMsg;
            ALLOCATE_MEMORY(responseMsg.payload, 512, "Unable to allocate memory for scoreboard message");
            responseMsg.type = MSG_ERR;
            strcpy(responseMsg.payload, "Server is full\n");
            responseMsg.size = strlen(responseMsg.payload);
            sendMessageToClient(responseMsg, newClientFd);

            free(responseMsg.payload);

            // The player fd must be closed so that the server won't try to communicate with this client in the future
            close(newClientFd);

            printf(RED("Server is full, access denied\n"));

            pthread_mutex_unlock(&scoresMutex);
            continue;
        }
        pthread_mutex_unlock(&scoresMutex);

        printf("Client accepted, socket fd is %d\n", newClientFd);

        // Initializing a new player
        Player* newPlayer = createPlayer(newClientFd);

        // Creating a new thread to handle the client requestes and responses
        pthread_create(&newPlayer->tid, NULL, handleClient, (void *)newPlayer);
    }
}
