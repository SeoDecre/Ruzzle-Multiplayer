#include "server.h"

int serverFd;
volatile ServerState currentState = WAITING_STATE;

pthread_mutex_t scoresMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loadRestOfDictionaryMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gameEndedCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t scoresListCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t CSVResultsCond = PTHREAD_COND_INITIALIZER;

PlayerList* playersList;
ScoresList scoresList;
Cell matrix[MATRIX_SIZE][MATRIX_SIZE];
TrieNode* trieRoot;
int isRestOfDictionaryAlreadyBeenLoaded;
int connectedClients;
int matchTime;
char* csvResult;
char* matrixFileName;
char* dictionaryFileName;

int isGameEnded;
int isScoresListReady;
int isCSVResultsScoreboardReady;

void cleanup() {
    // Socket server closure
    CHECK_NEGATIVE(close(serverFd), "Failed closing the server socket");

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
    freeTrie(trieRoot);
    free(dictionaryFileName);
    if (matrixFileName) free(matrixFileName);
    if (csvResult) free(csvResult);

    printf(RED("SERVER ENDED\n"));
}

void handleSigint() {
    cleanup();
    exit(0);
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
        matrixFileName ?  createNextMatrixFromFile(matrix, matrixFileName) : initRandomMatrix(matrix);
        printf("New matrix:\n");
        printMatrix(matrix);
    } else {
        currentState = WAITING_STATE;
        printf(YELLOW("Switched to WAITING_STATE\n"));

        // Initializating the scores list that will be filled by the client threads with their scores
        // We don't care if the playersList is empty, the scoresList will be initialized anyway
        // The scoresList memory allocation increases dynamically, adapting to possible registration in the meanwhile
        initializeScoresList(&scoresList);
        
        // Triggering the sendFinalResults threads
        // Even though there might not be registered players, we trigger the gameEndedCond so that aventual zombie playerScoreCollectorThread threads can be canceled
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

void parseMessage(char* buffer, Message* msg) {
    // Parsing of message type
    msg->type = buffer[0];

    // Parsing of message size
    memcpy(&msg->size, buffer + sizeof(char), sizeof(int));

    // Payload memory allocation and storing
    ALLOCATE_MEMORY(msg->payload, msg->size, "Unable to allocate memory for message payload\n");
    memcpy(msg->payload, buffer + sizeof(char) + sizeof(int), msg->size);
    msg->payload[msg->size] = '\0';

    // Printing parsed message
    printf("%c %d %s\n", msg->type, msg->size, msg->payload);
    // for (int i = 0; i < msg.size; i++) printf("%02x ", (unsigned char)msg.payload[i]);
    // printf("\n\n");
}

void sendMessageToClient(Message msg, int clientFd) {\
    // Allocating correct amount of bytes
    char* buffer;
    int totalSize = sizeof(char) + sizeof(int) + msg.size;
    ALLOCATE_MEMORY(buffer, totalSize, "Unable to allocate memory for response buffer\n");

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
    CHECK_NEGATIVE(write(clientFd, buffer, totalSize), "nella write");

    // Freeing temporary buffer
    free(buffer);
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

        // Resetting playersList scores and words list for next round
        for (int i = 0; i < playersList->size; i++) playersList->players[i]->score = 0;
        cleanPlayersListOfWords(playersList);

        // Unlock the mutex after accessing scoresList
        pthread_mutex_unlock(&scoresMutex);

        printf("Final results have been listed: %s\n", csvResult);

        // Notify each sendFinalResults thread the CSV results scoreboard is ready
        isCSVResultsScoreboardReady = 1;
        pthread_cond_broadcast(&CSVResultsCond);
    }

    return NULL;
}

void setResponseMsg(Message* responseMsg, char type, char* payload) {
    responseMsg->type = type;
    responseMsg->size = strlen(payload);
    strcpy(responseMsg->payload, payload);
}

void toLowercase(char *str) {
    if (str == NULL) return;

    while (*str != '\0') {
        if (*str >= 'A' && *str <= 'Z') *str = *str + ('a' - 'A');
        str++;
    }
}

void* playerScoreCollectorThread(void* player) {
    Player* playerPointer = (Player *)player;
    printf("PLAYER: %s - %d - score %d\n", playerPointer->name, playerPointer->fd, playerPointer->score);

    // The clientCommunicationThread threads use the scoresMutex to prevent race conditions accessing the playerPointer
    while (1) {
        pthread_mutex_lock(&scoresMutex);
        
        while (!isGameEnded) {
            // printf("%s is waiting for game to end...\n", playerPointer->name);
            // Now the scoresMutex gets unlocked and the thread waits till the gameEndedCond gets triggered by the main thread
            pthread_cond_wait(&gameEndedCond, &scoresMutex);
        }

        // Here the scoresMutex is locked, so that we can make safely access the scoresList shared list

        // Since this thread could be alive even though the related player has quitted, we check if that's the case and use this safe spot to clean the resources up
        if (playerPointer->fd < 0) {
            printf(RED("%s has quitted previously, playerScoreCollectorThread ended\n"), playerPointer->name);
    
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
        printf("%s adds %d to the scores list\n", playerPointer->name, playerPointer->score);

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
            // printf("%s is waiting for scorer thread to make ranks...\n", playerPointer->name);
            pthread_cond_wait(&CSVResultsCond, &scoresMutex);
        }
        
        // Prepare the response message to send to this specific client
        printf(GREEN("Sending rank results to %s\n"), playerPointer->name);
        Message responseMsg;
        ALLOCATE_MEMORY(responseMsg.payload, MAX_RANKS_BUFFER_BYTES, "Unable to allocate memory for scoreboard message");
        setResponseMsg(&responseMsg, MSG_PUNTI_FINALI, csvResult);
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

void* clientCommunicationThread(void* player) {
    Player* playerPointer = (Player *)player;
    int valread;
    char* buffer;
    ALLOCATE_MEMORY(buffer, MAX_BUFFER_BYTES, "Unable to allocate memory for requests buffer\n");

    // Updating connected clients counter
    pthread_mutex_lock(&scoresMutex);
    connectedClients++;
    printf(YELLOW("%d CLIENTS\n"), connectedClients);
    pthread_mutex_unlock(&scoresMutex);

    // Read incoming message from client
    while ((valread = read(playerPointer->fd, buffer, MAX_BUFFER_BYTES)) > 0) {
        printf("From client %d: ", playerPointer->fd);
        Message clientRequestMsg;
        parseMessage(buffer, &clientRequestMsg); // Also prints the parsed buffer content

        // Allocate memory for the response payload
        Message responseMsg;
        ALLOCATE_MEMORY(responseMsg.payload, MAX_BUFFER_BYTES, "Unable to allocate memory for response payload\n");

        pthread_mutex_lock(&scoresMutex);

        switch (clientRequestMsg.type) {
            case MSG_REGISTRA_UTENTE:
                if (isPlayerAlreadyRegistered(playersList, playerPointer->fd)) {
                    setResponseMsg(&responseMsg, MSG_ERR, "Already registered");
                    sendMessageToClient(responseMsg, playerPointer->fd);
                    free(responseMsg.payload);
                } else if (!isNicknameValid(playersList, clientRequestMsg.payload)) {
                    setResponseMsg(&responseMsg, MSG_ERR, "Invalid nickname");
                    sendMessageToClient(responseMsg, playerPointer->fd);
                    free(responseMsg.payload);
                } else {
                    // Adding player to players list
                    playerPointer->tid = pthread_self();
                    strcpy(playerPointer->name, clientRequestMsg.payload);
                    addPlayer(playersList, playerPointer);

                    setResponseMsg(&responseMsg, MSG_OK, "Game joined");
                    sendMessageToClient(responseMsg, playerPointer->fd);

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
                    pthread_create(&playerPointer->scorerTid, NULL, playerScoreCollectorThread, (void *)playerPointer);

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
                        responseMsg.size = strlen(responseMsg.payload);
                    } else {
                        setResponseMsg(&responseMsg, MSG_ERR, "You're not registered yet");
                    }

                    // Sending response to client
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
                toLowercase(clientRequestMsg.payload);

                // Replacing "Qu" with 'q' so that both the functions used to search the word in the matrix or in the trie don't have to
                replaceQu(clientRequestMsg.payload);

                // Deciding what to response to client
                if (!isPlayerAlreadyRegistered(playersList, playerPointer->fd)) {
                    setResponseMsg(&responseMsg, MSG_ERR, "You're not registered yet");
                } else if (currentState == WAITING_STATE) {
                    setResponseMsg(&responseMsg, MSG_ERR, "Waiting for match to start");
                } else if (didUserAlreadyGuessedWord(playersList, playerPointer->fd, clientRequestMsg.payload)) {
                    setResponseMsg(&responseMsg, MSG_PUNTI_PAROLA, "0\n");
                } else if (doesWordExistInMatrix(matrix, clientRequestMsg.payload)) {
                    // Loading words with more than average guessed word length from the dictionary
                    pthread_mutex_lock(&loadRestOfDictionaryMutex);
                    if (strlen(clientRequestMsg.payload) > AVERAGE_GUESS_WORD_LENGTH && !isRestOfDictionaryAlreadyBeenLoaded) {
                        loadRestOfDictionary(trieRoot, dictionaryFileName);
                        isRestOfDictionaryAlreadyBeenLoaded = 1;
                    }
                    pthread_mutex_unlock(&loadRestOfDictionaryMutex);

                    // Now we can check if the word is actually present in the dictionary
                    if (search(trieRoot, clientRequestMsg.payload)) {
                        int earnedPoints = strlen(clientRequestMsg.payload);
                        printf(GREEN("Word %s exists, +%d\n"), clientRequestMsg.payload, (int)strlen(clientRequestMsg.payload));

                        // Updating Player score and list of guessed words
                        int updatedScore = getPlayerScore(playersList, playerPointer->fd) + earnedPoints;
                        updatePlayerScore(playersList, playerPointer->fd, updatedScore);
                        addWordToPlayer(playersList, playerPointer->fd, clientRequestMsg.payload);

                        // Sending how much points the given word was as string
                        char pointsString[2];
                        sprintf(pointsString, "%d", earnedPoints);
                        strcpy(responseMsg.payload, pointsString);

                        // Printing players list to see updated score
                        printPlayerList(playersList);

                        setResponseMsg(&responseMsg, MSG_PUNTI_PAROLA, pointsString);
                    } else {
                        setResponseMsg(&responseMsg, MSG_ERR, "Invalid word");
                    }
                } else {
                    setResponseMsg(&responseMsg, MSG_ERR, "Invalid word");
                }

                // Sending response to client
                sendMessageToClient(responseMsg, playerPointer->fd);

                free(responseMsg.payload);
                break;
            default:
                break;
        }

        free(clientRequestMsg.payload);

        pthread_mutex_unlock(&scoresMutex);
    }

    printf(RED("Client %s disconnected\n"), playerPointer->name);

    // I choosed to don't pthread_delete the playerScoreCollectorThread thread directly here
    // In general, we don't really want to violently kill a thread, but instead we want to ask it to terminate
    // That way we can be sure that the child is quitting at a safe spot and all its resources are cleaned up
    // In this case, we let it alive, so that it will exit by itself as soon as it realizes the player has quitted
    pthread_mutex_lock(&scoresMutex);
    removePlayer(playersList, playerPointer);
    connectedClients--;
    printf(YELLOW("%d CLIENTS\n"), connectedClients);
    pthread_mutex_unlock(&scoresMutex);

    // Printing players list after disconnection
    printPlayerList(playersList);

    // The player fd must be closed so that the server won't try to communicate with this client in the future
    if (playerPointer->fd != -1) close(playerPointer->fd);

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

    // serverAddress structure initialization
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    CHECK_NEGATIVE(inet_pton(AF_INET, serverName, &serverAddress.sin_addr), "Invalid address");

    // Socket creation, binding and listen
    SYSC(serverFd, socket(AF_INET, SOCK_STREAM, 0), "Unable to create socket socket");
    SYSC(retValue, bind(serverFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)), "Enable to bind socket");
    SYSC(retValue, listen(serverFd, MAX_CLIENTS), "Unable to start listen");

    // Filling matrix up using a new matrix file name if passed, standard one if not
    if (passedMatrixFileName) {
        ALLOCATE_MEMORY(matrixFileName, strlen(passedMatrixFileName) + 1, "Unable to allocate memory for matrix file name string\n");
        strcpy(matrixFileName, passedMatrixFileName);
    }
    
    // Loading dictionary using a new dictionary file name if passed, standard one if not
    ALLOCATE_MEMORY(dictionaryFileName, newDictionaryFile ? strlen(newDictionaryFile) + 1 : strlen(STD_DICTIONARY_FILENAME) + 1, "Unable to allocate memory for dictionary file name string\n");
    strcpy(dictionaryFileName, newDictionaryFile ? newDictionaryFile : STD_DICTIONARY_FILENAME);
    
    // Loading dictionary function doesn't take that much time to be executed, however, a new thread could be used to load it
    // But since we start the server at the WAITING_STATE, the function has plenty of time to be executed before actually being used
    trieRoot = createNode();
    loadFirstPartOfDictionary(trieRoot, dictionaryFileName);

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
            ALLOCATE_MEMORY(responseMsg.payload, 32, "Unable to allocate memory for scoreboard message");
            setResponseMsg(&responseMsg, MSG_ERR, "Server is full");
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
        pthread_create(&newPlayer->tid, NULL, clientCommunicationThread, (void *)newPlayer);
    }
}
