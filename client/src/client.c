#include "client.h"

int serializeMessage(const Message* msg, char** buffer) {
    int totalSize = sizeof(char) + sizeof(int) + msg->size;
    *buffer = (char*)malloc(totalSize);
    if (*buffer == NULL) {
        perror("Failed to allocate memory for buffer");
        exit(EXIT_FAILURE);
    }

    (*buffer)[0] = msg->type;
    memcpy(*buffer + sizeof(char), &(msg->size), sizeof(int));
    memcpy(*buffer + sizeof(char) + sizeof(int), msg->payload, msg->size);

    return totalSize;
}

Message parseMessage(const char* buffer) {
    Message msg;
    msg.type = buffer[0];
    memcpy(&msg.size, buffer + sizeof(char), sizeof(int));

    msg.payload = (char*)malloc(msg.size);
    if (msg.payload == NULL) {
        perror("Failed to allocate memory for payload");
        exit(EXIT_FAILURE);
    }
    memcpy(msg.payload, buffer + sizeof(char) + sizeof(int), msg.size);

    return msg;
}

void processCommand(const char* command, const char* content, char* serviceReturnMsg, int clientFd, pthread_mutex_t* mutex) {
    Message msg;
    int retvalue;
    char* buffer = NULL;

    if (strcmp(command, "aiuto") == 0) {
        strcpy(serviceReturnMsg, "Comandi disponibili:\naiuto\nmatrice\np <parola>\nfine\n");
    } else {
        if (strcmp(command, "registra_utente") == 0) {
            msg.type = MSG_REGISTRA_UTENTE;
        } else if (strcmp(command, "matrice") == 0) {
            msg.type = MSG_MATRICE;
        } else if (strcmp(command, "p") == 0) {
            msg.type = MSG_PAROLA;
        } else if (strcmp(command, "fine") == 0) {
            free(serviceReturnMsg);
            pthread_mutex_destroy(mutex);
            SYSC(retvalue, close(clientFd), "close");
            exit(EXIT_SUCCESS);
        } else {
            strcpy(serviceReturnMsg, "Comando non riconosciuto. Digita 'aiuto' per vedere i comandi disponibili.\n");
            return;
        }

        msg.size = strlen(content);
        msg.payload = (char*)content;
        int totalSize = serializeMessage(&msg, &buffer);

        // Check what we're sending
        printf("SENDING %d bytes: \n", totalSize);
        for (int i = 0; i < totalSize; i++) printf("%02x ", (unsigned char)buffer[i]);
        printf("\n");

        SYSC(retvalue, write(clientFd, buffer, totalSize), "write");

        free(buffer);
    }
}

void* sendMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;

    // The clientInput string will contain a command and, eventually, the message content
    // So we allocate memory for both of them
    char* command = (char*)malloc(16); // The longest possible command is 15 bytes long (registra_utente)
    char* content = (char*)malloc(21); // The longest possible content is 20 bytes long (nickname length)

    // Parse client input
    sscanf(params->clientInput, "%15s%*c%20[^\n]", command, content);

    pthread_mutex_lock(&(params->mutex));
    processCommand(command, content, params->serviceReturnMsg, params->clientFd, &(params->mutex));
    pthread_mutex_unlock(&(params->mutex));
    
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
            break;
        case MSG_TEMPO_PARTITA:
            *params->timeLeft = atoi(msg->payload);
            // strcpy(params->serviceReturnMsg, "In game");
            break;
        case MSG_TEMPO_ATTESA:
            *params->timeLeft = atoi(msg->payload);
            strcpy(params->serviceReturnMsg, "Waiting for match to start");
            initMatrix(params->matrix);
            break;
        default:
            strcpy(params->serviceReturnMsg, msg->payload);
            break;
    }
}

void* handleReceivedMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    char* buffer = (char*)malloc(1024);
    int retvalue;

    while (1) {
        // Reading server response through file descriptor
        SYSC(retvalue, read(params->clientFd, buffer, 1024), "nella read"); // Lettura sul fd client

        // Parsing obtained buffer
        Message msg = parseMessage(buffer); // Also prints its content

        pthread_mutex_lock(&(params->mutex));
        processReceivedMessage(&msg, params);
        pthread_mutex_unlock(&(params->mutex));
        
        free(msg.payload);
        displayGameShell(params);
    }

    return NULL;
}

void client(int port) {
    struct sockaddr_in server_addr;
    int client_fd, retvalue, score = 0, timeLeft = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    // Variable used to store the matrix
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];

    // Input message of the client
    char* clientInput = (char*)malloc(35);

    // Info message returned by server or other command output
	char* serviceMsg = (char*)malloc(100);

    // socket address struct initialization, storing address and port used to create the connection
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    // Socket creation
    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Trying connection with the server
    SYSC(retvalue, connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella connect");

    // Storing a connection info message so that it will displayed in the shell
    strcpy(serviceMsg, GREEN("Connected to the server"));

    // Initialization of the matrix to "empty" chars
    // It will be filled eventually with a server request
    initMatrix(matrix);
    
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

    // Create thread for receiving messages from server
    // The passed function creates a loop to continuously read from the server fd through the socket
    pthread_t receiverTid;
    pthread_create(&receiverTid, NULL, handleReceivedMessage, (void *)&params);

    // Main loop
    while (1) {
        displayGameShell(&params);

        // Read user input
        if (scanf("%69[^\n]%*c", clientInput) != 1) {
            // Skip to next iteration
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
