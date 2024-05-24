#include "client.h"
#include "colors.h"

int serializeMessage(const Message* msg, char* buffer) {
    int totalSize = sizeof(char) + sizeof(int) + msg->size;

    // Allocate memory for the buffer
    buffer = (char*)malloc(totalSize);
    if (buffer == NULL) {
        perror("Failed to allocate memory for buffer");
        exit(EXIT_FAILURE);
    }

    // Copy 'type' field into the buffer
    buffer[0] = msg->type;

    // Copy 'size' field into the buffer
    memcpy(buffer + sizeof(char), &(msg->size), sizeof(int));

    // Copy 'payload' into the buffer
    memcpy(buffer + sizeof(char) + sizeof(int), msg->payload, msg->size);

    return totalSize;  // Return the total size of the serialized message
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

    // Printing parsed message
    // printf("%c %d %s\n", msg.type, msg.size, msg.payload);

    return msg;
}

// Modified sendMessage function to accept ThreadParams
void* sendMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;

    char* serviceReturnMsg = params->serviceReturnMsg;
    char* clientInput = params->clientInput;
    int* timeLeft = params->timeLeft;
    Cell (*matrix)[MATRIX_SIZE] = params->matrix;
    int clientFd = params->clientFd;
    int* score = params->score;
    pthread_mutex_t* mutex = &(params->mutex);

    // Server write returned value
    int retvalue;

    // The clientInput string will contain a command and, eventually, the message content
    // So we allocate memory for both of them
    char* command = (char*)malloc(16); // The longest possible command is 15 bytes long (registra_utente)
    char* content = (char*)malloc(21); // The longest possible content is 20 bytes long (nickname length)

    // Checking for memory allocation errors
    if (command == NULL || content == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Initialize buffers to avoid undefined behavior
    memset(command, 0, 16);
    memset(content, 0, 21);

    // Parse client input
    sscanf(clientInput, "%15s%*c%20[^\n]", command, content);

    /*
    printf("COMMAND %s: ", command);
    for (int i = 0; i < 15; i++) printf(BLUE("%02x "), (unsigned char)command[i]);
    printf("\nCONTENT %s: ", content);
    for (int i = 0; i < 20; i++) printf(BLUE("%02x "), (unsigned char)content[i]);
    printf("\n");
    */

    // Freeing service return message memory content so that it can be correctly filled with new content
    // if (strlen(serviceReturnMsg) > 0) free(serviceReturnMsg);

    pthread_mutex_lock(mutex);

    // Handle different commands
    if (strcmp(command, "aiuto") == 0) {
        printf("AIUTO");
        strcpy(serviceReturnMsg, "Comandi disponibili:\naiuto\nmatrice\np <parola>\nfine\n");
    } else if (strcmp(command, "registra_utente") == 0) {
        // strcpy(serviceReturnMsg, "Sent request to server\n");
        // User wants to register to the game
        Message msg = {.type = MSG_REGISTRA_UTENTE, .size = strlen(content), .payload = content};

        // Allocating correct amount of bytes for the message buffer to send to the server
        int totalSize = sizeof(char) + sizeof(int) + msg.size;
        char* buffer = (char*)malloc(totalSize);

        // Serializing the message structure into the buffer string
        buffer[0] = msg.type;
        memcpy(buffer + sizeof(char), &msg.size, sizeof(int));
        memcpy(buffer + sizeof(char) + sizeof(int), msg.payload, msg.size);

        // Check what we're sending
        printf("SENDING %d bytes: ", totalSize);
        for (int i = 0; i < totalSize; i++) printf(BLUE("%02x "), (unsigned char)buffer[i]);
        printf("\n");

        // Send the serialized message to the server
        SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");

        // Free the allocated buffer after sending
        free(buffer);
    } else if (strcmp(command, "matrice") == 0) {
        Message msg = {.type = MSG_MATRICE, .size = strlen(content), .payload = content};
        
        // Allocating correct amount of bytes for the message buffer to send to the server
        int totalSize = sizeof(char) + sizeof(int) + msg.size;
        char* buffer = (char*)malloc(totalSize);

        // Serializing the message structure into the buffer string
        buffer[0] = msg.type;
        memcpy(buffer + sizeof(char), &msg.size, sizeof(int));
        memcpy(buffer + sizeof(char) + sizeof(int), msg.payload, msg.size);

        // Check what we're sending
        printf("SENDING %d bytes: ", totalSize);
        for (int i = 0; i < totalSize; i++) printf(BLUE("%02x "), (unsigned char)buffer[i]);
        printf("\n");

        // Send the serialized message to the server
        SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");

        // Free the allocated buffer after sending
        free(buffer);
    } else if (strcmp(command, "p") == 0) {
        // User wants to send a word
        Message msg = {.type = MSG_PAROLA, .size = strlen(content), .payload = content};

        // Allocating correct amount of bytes for the message buffer to send to the server
        int totalSize = sizeof(char) + sizeof(int) + msg.size;
        char* buffer = (char*)malloc(totalSize);

        // Serializing the message structure into the buffer string
        buffer[0] = msg.type;
        memcpy(buffer + sizeof(char), &msg.size, sizeof(int));
        memcpy(buffer + sizeof(char) + sizeof(int), msg.payload, msg.size);

        // Check what we're sending
        printf("SENDING %d bytes: ", totalSize);
        for (int i = 0; i < totalSize; i++) printf(BLUE("%02x "), (unsigned char)buffer[i]);
        printf("\n");

        // Send the serialized message to the server
        SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");

        // Free the allocated buffer after sending
        free(buffer);
    } else if (strcmp(command, "fine") == 0) {
        // Freeing command and memory content
        free(command);
        free(content);
        free(serviceReturnMsg);
        SYSC(retvalue, close(clientFd), "nella close");
        exit(EXIT_SUCCESS);
    } else {
        strcpy(serviceReturnMsg, "Comando non riconosciuto. Digita 'aiuto' per vedere i comandi disponibili.\n");
    }

    pthread_mutex_unlock(mutex);

    // Freeing command and memory content
    // printf("FINITA\n");
    free(command);
    free(content);
    fflush(stdout);
    
    return NULL;
}

void displayGameShell(ThreadParams* params) {
    // Clear the screen and move cursor to top-left
    printf("\033[2J"); // ANSI escape sequence to clear screen
    printf("\033[H");  // ANSI escape sequence to move cursor to top-left

    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    printf(YELLOW("Time left: %d\n"), *params->timeLeft);
    
    // Printing the matrix
    printMatrix(params->matrix);

    // Print the current score
    printf(CYAN("Score: %d\n"), *params->score);
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");

    // Printing shell service messages, if not NULL or invalid
    printf(MAGENTA("\n%s\n"), params->serviceReturnMsg);

    // Shell input
    printf("\n>> ");
    fflush(stdout);
}

void processReceivedMessage(Message* msg, ThreadParams* params) {
    switch (msg->type) {
        case MSG_MATRICE:
            memcpy(params->matrix, msg->payload, msg->size);
            break;
        case MSG_PUNTI_PAROLA:
            *params->score += atoi(msg->payload);
            sprintf(params->serviceReturnMsg, (atoi(msg->payload) > 0) ? GREEN("Nice! +%d") : RED("Invalid word"), atoi(msg->payload));
            // sprintf(params->serviceReturnMsg, "Points: %d", *params->score);
            break;
        case MSG_TEMPO_PARTITA:
        case MSG_TEMPO_ATTESA:
            *params->timeLeft = atoi(msg->payload);
            sprintf(params->serviceReturnMsg, "Time left: %d", *params->timeLeft);
            break;
        default:
            strcpy(params->serviceReturnMsg, msg->payload);
            break;
    }
}

// Modified handleReceivedMessage function to accept ThreadParams
void* handleReceivedMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int clientFd = params->clientFd;
    char* buffer = (char*)malloc(1024);
    int retvalue;

    while (1) {
        // Reading server response through file descriptor
        SYSC(retvalue, read(clientFd, buffer, 1024), "nella read"); // Lettura sul fd client

        // Parsing obtained buffer
        Message msg = parseMessage(buffer); // Also prints its content

        pthread_mutex_lock(&(params->mutex));
        processReceivedMessage(&msg, params);
        pthread_mutex_unlock(&(params->mutex));
        
        free(msg.payload);
        displayGameShell(params);

        /*
        switch (msg.type) {
            case MSG_MATRICE:
                // Extract matrix from buffer
                memcpy(matrix, msg.payload, msg.size); // TODO: Remember to change if doesn't work
                break;
            case MSG_PUNTI_PAROLA:
                int wordPoints = atoi(msg.payload);
                *score += wordPoints;

                char* infoMsg = (char*)malloc(20);
                sprintf(infoMsg, (wordPoints > 0) ? GREEN("Nice! +%d") : RED("Invalid word"), wordPoints);
                strcpy(serviceReturnMsg, infoMsg);
                break;
            case MSG_TEMPO_PARTITA:
                *timeLeft = atoi(msg.payload);
                char* infoMsg1 = (char*)malloc(20);
                sprintf(infoMsg1, "In game: %d", *timeLeft);
                strcpy(serviceReturnMsg, infoMsg1);
                break;
            case MSG_TEMPO_ATTESA:
                *timeLeft = atoi(msg.payload);
                char* infoMsg2 = (char*)malloc(20);
                sprintf(infoMsg2, "Waiting: %d", *timeLeft);
                strcpy(serviceReturnMsg, infoMsg2);
                break;
            default:
                // Stampa della risposta del server
                strcpy(serviceReturnMsg, msg.payload);
                break;
        }
        */
    }

    return NULL;
}

/*
Main client function
Creates a new thread to continuously listen to the server
Uses the main thread to create a shell-alike terminal for user prompts
*/
void client(int port) {
    struct sockaddr_in server_addr;
    int client_fd, retvalue, i, j, score = 0, timeLeft = 0;
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];

    // Input message of the client
    char* clientInput = (char*)malloc(35);

    // Info message returned by server or other command output
	char* serviceMsg = (char*)malloc(100);

    // server_addr struct initialization (storing address and port used to create the connection)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    // Socket creation
    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Connect
    SYSC(retvalue, connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella connect");

    // Storing a connection info message so that it will displayed in the shell
    strcpy(serviceMsg, GREEN("Connected to the server"));

    // Initialization of the matrix to empty chars
    initMatrix(matrix);

    // Mutex variable
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Thread parameters
    ThreadParams params = {
        .clientFd = client_fd,
        .clientInput = clientInput,
        .serviceReturnMsg = serviceMsg,
        .matrix = matrix,
        .score = &score,
        .timeLeft = &timeLeft,
        .mutex = mutex
    };

    // Create threads for sending and receiving messages
    pthread_t receiverTid;
    pthread_create(&receiverTid, NULL, handleReceivedMessage, (void *)&params);

    while (1) {
        displayGameShell(&params);

        // Read user input
        if (scanf("%69[^\n]%*c", clientInput) != 1) {
            printf("Error reading input.\n");
            continue;
        }

        sendMessage((void *)&params);
    }

    // Freeing memory
    free(clientInput);
    free(serviceMsg);
    pthread_mutex_destroy(&(params.mutex));

    // Socket closure
    SYSC(retvalue, close(client_fd), "nella close");
}