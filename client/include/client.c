#include "client.h"

#define RED(string) "\x1b[31m" string "\x1b[0m"
#define GREEN(string) "\x1b[32m" string "\x1b[0m"
#define YELLOW(string) "\x1b[33m" string "\x1b[0m"
#define BLUE(string) "\x1b[34m" string "\x1b[0m"
#define MAGENTA(string) "\x1b[35m" string "\x1b[0m"
#define CYAN(string) "\x1b[36m" string "\x1b[0m"

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
    printf("%c %d %s\n", msg.type, msg.size, msg.payload);

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

    while (1) {
        // Read user input
        if (scanf("%69[^\n]%*c", clientInput) != 1) {
            printf("Error reading input.\n");
            return NULL;
        }

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

        printf("COMMAND %s: ", command);
        for (int i = 0; i < 15; i++) printf(BLUE("%02x "), (unsigned char)command[i]);
        printf("\nCONTENT %s: ", content);
        for (int i = 0; i < 20; i++) printf(BLUE("%02x "), (unsigned char)content[i]);
        printf("\n");

        // Freeing service return message memory content so that it can be correctly filled with new content
        // if (strlen(serviceReturnMsg) > 0) free(serviceReturnMsg);

        // Handle different commands
        if (strcmp(command, "aiuto") == 0) {
            strcpy(serviceReturnMsg, "Comandi disponibili:\naiuto\nmatrice\np <parola>\nfine\n");
        } else if (strcmp(command, "registra_utente") == 0) {
            strcpy(serviceReturnMsg, "Sent request to server\n");
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

        // Freeing command and memory content
        printf("FINITA\n");
        free(command);
        free(content);
    }
    
    return NULL;
}

// Modified handleReceivedMessage function to accept ThreadParams
void* handleReceivedMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int clientFd = params->clientFd;
    Cell (*matrix)[MATRIX_SIZE] = params->matrix;
    char* serviceReturnMsg = params->serviceReturnMsg;
    int* score = params->score;
    int* timeLeft = params->timeLeft;

    while (1) {
        // Clear the screen and move cursor to top-left
        printf("\033[2J"); // ANSI escape sequence to clear screen
        printf("\033[H");  // ANSI escape sequence to move cursor to top-left

        printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
        printf(YELLOW("Time left: %d\n"), *timeLeft);

        // Printing the matrix
        printMatrix(matrix);

        // Print the current score
        printf(CYAN("Score: %d\n"), *score);
        printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");

        // Printing shell service messages, if not NULL or invalid
        printf(MAGENTA("\n%s\n"), serviceReturnMsg);

        // Shell input
        printf("\n>> ");
        fflush(stdout);

        // Temporary buffer for receiving client data
        int retvalue;
        char* buffer = (char*)malloc(1024);

        // Reading server response through file descriptor
        SYSC(retvalue, read(clientFd, buffer, 1024), "nella read"); // Lettura sul fd client

        // Parsing obtained buffer
        printf("From server: ");
        Message msg = parseMessage(buffer); // Also prints its content

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

        free(buffer);
    }

    return NULL;
}

void client(int port) {
    struct sockaddr_in server_addr;
    int client_fd, retvalue, i, j;
    int score = 0;
    int timeLeft = 0;
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];

    // Input message of the client
    char* clientInput = (char*)malloc(35);

    // Info message returned by server or other command output
	char* serviceMsg = (char*)malloc(100);

    // Check for malloc errors
    if (clientInput == NULL || serviceMsg == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // server_addr struct initialization (storing address and port used to create the connection)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    // Socket creation
    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Connect
    SYSC(retvalue, connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella connect");

    // Set socket to non-blocking mode
    // int flags = fcntl(client_fd, F_GETFL, 0);
    // if (flags == -1) printf("FLAG ERROR");
    // fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    // Storing a connection info message so that it will displayed in the shell
    strcpy(serviceMsg, GREEN("Connected to the server"));

    // Initialization of the matrix to empty chars
    initMatrix(matrix);

    // Thread parameters
    ThreadParams params;
    params.clientFd = client_fd;
    params.clientInput = clientInput;
    params.serviceReturnMsg = serviceMsg;
    params.matrix = matrix;
    params.score = &score;
    params.timeLeft = &timeLeft;

    pthread_t senderTid, receiverTid;

    // Create threads for sending and receiving messages
    pthread_create(&senderTid, NULL, sendMessage, (void *)&params);
    pthread_create(&receiverTid, NULL, handleReceivedMessage, (void *)&params);

    // Wait for them to end
    pthread_join(senderTid, NULL);
    pthread_join(receiverTid, NULL);

    // Freeing memory
    free(clientInput);
    free(serviceMsg);

    // Socket closure
    SYSC(retvalue, close(client_fd), "nella close");
}