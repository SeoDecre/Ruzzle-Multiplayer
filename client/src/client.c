#include "client.h"

int serializeMessage(const Message* msg, char** buffer) {
    int totalSize = sizeof(char) + sizeof(int) + msg->size;
    ALLOCATE_MEMORY(*buffer, totalSize, "Failed to allocate memory for buffer");
    
    (*buffer)[0] = msg->type;
    memcpy(*buffer + sizeof(char), &(msg->size), sizeof(int));
    memcpy(*buffer + sizeof(char) + sizeof(int), msg->payload, msg->size);

    return totalSize;
}

void processCommand(const char* command, const char* content, ThreadParams* params) {
    Message msg;
    int retValue;
    char* buffer = NULL;

    if (strcmp(command, "aiuto") == 0) {
        strcpy(params->shellInfoMessage, "Comandi disponibili:\naiuto\nmatrice\np <parola>\nfine\n");
        displayGameShell(params);
    } else {
        CHECK_NULL(content, "Content is null");

        if (strcmp(command, "registra_utente") == 0) {
            msg.type = MSG_REGISTRA_UTENTE;
        } else if (strcmp(command, "matrice") == 0) {
            msg.type = MSG_MATRICE;
        } else if (strcmp(command, "p") == 0) {
            msg.type = MSG_PAROLA;
        } else {
            strcpy(params->shellInfoMessage, "Comando non riconosciuto. Digita 'aiuto' per vedere i comandi disponibili.");
            displayGameShell(params);
            return;
        }

        msg.size = strlen(content);
        msg.payload = (char*)content;
        int totalSize = serializeMessage(&msg, &buffer);

        // Check what we're sending
        // printf("SENDING %d bytes: \n", totalSize);
        // for (int i = 0; i < totalSize; i++) printf("%02x ", (unsigned char)buffer[i]);
        // printf("\n");

        SYSC(retValue, write(params->clientFd, buffer, totalSize), "write");
        free(buffer);
    }
}

void sendMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;

    // The clientInput string will contain a command and, eventually, the message content
    char* command;
    char* content;

    // The longest possible command is 15 bytes long (registra_utente), so 32 bytes are enough
    ALLOCATE_MEMORY(command, 32, "Failed to allocate memory");
    // The longest possible content is 20 bytes long (nickname length), so 32 bytes are enough
    ALLOCATE_MEMORY(content, 32, "Failed to allocate memory");

    memset(command, 0, 32);
    memset(content, 0, 32);

    // Parse client input
    if (sscanf(params->clientInput, "%31s%*c%31[^\n]", command, content) < 1) {
        free(command);
        free(content);
        return;
    }

    pthread_mutex_lock(&(params->mutex));
    processCommand(command, content, params);
    pthread_mutex_unlock(&(params->mutex));

    free(command);
    free(content);
}

void displayGameShell(ThreadParams* params) {
    // Clear the screen and move cursor to top-left
    printf("\033[2J"); // ANSI escape sequence to clear screen
    printf("\033[H");  // ANSI escape sequence to move cursor to top-left

    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    printf(YELLOW("Time left: %d\n\n"), *params->timeLeft);
    
    // Printing the matrix
    printMatrix(params->matrix);

    // Printing the final scoreboard, when ready
    if (strcmp(params->finalScoreboard, "") != 0) printf("%s\n", params->finalScoreboard);
    
    // Print the current score
    printf(CYAN("\nScore: %d\n"), *params->score);
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");

    // Printing shell service messages, if not NULL or invalid
    printf(MAGENTA("\n%s\n"), params->shellInfoMessage);

    // Shell input
    printf("\n>> ");
    fflush(stdout);
}

void parseAndMemorizeScoreboard(char* finalScoreboard, char* msgPayload) {
    // Initialize variables
    char* token;
    int rank = 1;
    
    // Start parsing the msgPayload
    token = strtok(msgPayload, ",");

    strcpy(finalScoreboard, RED("FINAL SCOREBOARD\n"));
    
    // Loop through the tokens
    while (token != NULL) {
        // Get the name
        char name[50];
        strcpy(name, token);
        
        // Get the next token for the score
        token = strtok(NULL, ",");
        if (token == NULL) break; // Check to avoid segmentation fault
        
        // Get the score
        int score = atoi(token);
        
        // Format and append to finalScoreboard
        char formattedLine[100];
        snprintf(formattedLine, sizeof(formattedLine), "%d° %s - score: %d\n", rank, name, score);
        strcat(finalScoreboard, formattedLine);
        
        // Increment the rank
        rank++;
        
        // Get the next name
        token = strtok(NULL, ",");
    }
}

void processReceivedMessage(Message* msg, ThreadParams* params) {
    switch (msg->type) {
        case MSG_MATRICE:
            memcpy(params->matrix, msg->payload, msg->size);
            strcpy(params->finalScoreboard, "");
            break;
        case MSG_PUNTI_PAROLA:
            *params->score += atoi(msg->payload);
            sprintf(params->shellInfoMessage, (atoi(msg->payload) > 0) ? GREEN("Nice! +%d") : RED("Invalid word"), atoi(msg->payload));
            break;
        case MSG_TEMPO_PARTITA:
            *params->timeLeft = atoi(msg->payload);
            // strcpy(params->shellInfoMessage, "In game");
            break;
        case MSG_TEMPO_ATTESA:
            *params->timeLeft = atoi(msg->payload);
            strcpy(params->shellInfoMessage, "Waiting for match to start");
            initMatrix(params->matrix);
            break;
        case MSG_PUNTI_FINALI:
            initMatrix(params->matrix);
            parseAndMemorizeScoreboard(params->finalScoreboard, msg->payload);
            *params->score = 0;
            break;
        default: // MSG_OK and MSG_ERR
            strcpy(params->shellInfoMessage, msg->payload);
            break;
    }
}

void parseServerMessage(Message* msg, const char* buffer) {
    msg->type = buffer[0];
    memcpy(&msg->size, buffer + sizeof(char), sizeof(int));

    ALLOCATE_MEMORY(msg->payload, msg->size + 1, "Failed to allocate memory for payload");
    memcpy(msg->payload, buffer + sizeof(char) + sizeof(int), msg->size);
    msg->payload[msg->size] = '\0';
}

void* handleReceivedMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int retValue;
    
    while (1) {
        // Reading server response through file descriptor
        SYSC(retValue, read(params->clientFd, params->serverResponse, MAX_SERVICE_RETURN_MSG_SIZE), "Unable to read server message");

        // Parsing obtained buffer
        Message serverResponseMsg;
        parseServerMessage(&serverResponseMsg, params->serverResponse); // Also prints its content
        
        pthread_mutex_lock(&(params->mutex));
        processReceivedMessage(&serverResponseMsg, params);
        pthread_mutex_unlock(&(params->mutex));
        
        free(serverResponseMsg.payload);
        displayGameShell(params);

        if (retValue <= 0) {
            *params->connectionClosed = 1;
            return NULL;
        }
    }
    
    return NULL;
}

void client(char* serverName, int port) {
    struct sockaddr_in serverAddr;
    int clientFd, retValue, score = 0, timeLeft = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    volatile sig_atomic_t connectionClosed = 0;

    // Variable used to store the matrix
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];

    // Input message of the client
    char* clientInput;
    ALLOCATE_MEMORY(clientInput, 64, "Failed to allocate memory for client input message");

    // Server response buffer
    char* serverResponse;
    ALLOCATE_MEMORY(serverResponse, MAX_SERVICE_RETURN_MSG_SIZE, "Failed to allocate memory for server response buffer");

    // Payload info message returned by server or other command output
    char* shellInfoMessage;
    ALLOCATE_MEMORY(shellInfoMessage, 100, "Failed to allocate memory for service info message");

    // Server response buffer
    char* finalScoreboard;
    ALLOCATE_MEMORY(finalScoreboard, MAX_SERVICE_RETURN_MSG_SIZE, "Failed to allocate memory for final scoreboard");

    memset(clientInput, 0, 64);
    memset(serverResponse, 0, MAX_SERVICE_RETURN_MSG_SIZE);
    memset(shellInfoMessage, 0, 100);
    memset(finalScoreboard, 0, MAX_SERVICE_RETURN_MSG_SIZE);

    // socket address struct initialization, storing address and port used to create the connection
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    CHECK_NEGATIVE(inet_pton(AF_INET, serverName, &serverAddr.sin_addr), "Invalid address");
    
    // Socket creation
    SYSC(clientFd, socket(AF_INET, SOCK_STREAM, 0), "Unable to create the socket");

    // Trying connection with the server
    SYSC(retValue, connect(clientFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)), "Unable to connect to the server");

    // Storing a connection info message so that it will displayed in the shell
    strcpy(shellInfoMessage, GREEN("Connected to the server"));

    // Initialization of the matrix to "empty" chars
    // It will be filled eventually with a server request
    initMatrix(matrix);

    // Thread parameters
    ThreadParams params = {
        .clientFd = clientFd,
        .clientInput = clientInput,
        .serverResponse = serverResponse,
        .shellInfoMessage = shellInfoMessage,
        .finalScoreboard = finalScoreboard,
        .matrix = matrix,
        .score = &score,
        .timeLeft = &timeLeft,
        .mutex = mutex,
        .connectionClosed = &connectionClosed
    };

    // Create thread for receiving messages from server
    // The passed function creates a loop to continuously read from the server fd through the socket
    pthread_t receiverTid;
    if (pthread_create(&receiverTid, NULL, handleReceivedMessage, (void *)&params) != 0) {
        perror("Thread creation failed");
        free(clientInput);
        free(serverResponse);
        free(shellInfoMessage);
        free(finalScoreboard);
        close(clientFd);
        exit(EXIT_FAILURE);
    }

    // Displaying game shell (will be updated accordingly to server responses)
    displayGameShell(&params);

    // Main loop
    while (!connectionClosed) {
        // Read user input
        // I'm using fgets instead of scanf because of how scanf handles the input
        // When the scanf receives only the "Enter" key it does not read any input and leaves the newline character in the input buffer, making the loop not behave as expected
        if (fgets(clientInput, 64, stdin) == NULL) continue;

        // Remove the newline character if present
        clientInput[strcspn(clientInput, "\n")] = '\0';

        // Exit if user wants to end the session
        if (strcmp(clientInput, "fine") == 0) break;

        // Otherwise send the written message
        sendMessage((void *)&params);
    }

    // Stopping and removing thread
    pthread_cancel(receiverTid);
    pthread_join(receiverTid, NULL);
    pthread_mutex_destroy(&mutex);

    strcpy(shellInfoMessage, "Server terminated\n");
    displayGameShell(&params);

    // Freeing memory
    free(clientInput);
    free(serverResponse);
    free(shellInfoMessage);
    free(finalScoreboard);

    // Socket closure
    SYSC(retValue, close(clientFd), "Unable to close the client connection");
}
