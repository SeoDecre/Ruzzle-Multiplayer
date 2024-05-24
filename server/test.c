#include "client.h"

#define RED(string) "\x1b[31m" string "\x1b[0m"
#define GREEN(string) "\x1b[32m" string "\x1b[0m"
#define YELLOW(string) "\x1b[33m" string "\x1b[0m"
#define BLUE(string) "\x1b[34m" string "\x1b[0m"
#define MAGENTA(string) "\x1b[35m" string "\x1b[0m"
#define CYAN(string) "\x1b[36m" string "\x1b[0m"

#define MAX_COMMAND_LENGTH 16
#define MAX_CONTENT_LENGTH 21

// Function Prototypes

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

void* sendMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    char command[MAX_COMMAND_LENGTH];
    char content[MAX_CONTENT_LENGTH];

    parseClientInput(params->clientInput, command, content);

    pthread_mutex_lock(&(params->mutex));
    handleCommand(params, command, content);
    pthread_mutex_unlock(&(params->mutex));

    return NULL;
}

void* handleReceivedMessage(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    int clientFd = params->clientFd;

    while (1) {
        char* buffer = (char*)malloc(1024);
        int retvalue;

        SYSC(retvalue, read(clientFd, buffer, 1024), "nella read");

        Message msg = parseMessage(buffer);

        pthread_mutex_lock(&(params->mutex));
        switch (msg.type) {
            case MSG_MATRICE:
                memcpy(params->matrix, msg.payload, msg.size);
                break;
            case MSG_PUNTI_PAROLA:
                *params->score += atoi(msg.payload);
                snprintf(params->serviceReturnMsg, 100, (atoi(msg.payload) > 0) ? GREEN("Nice! +%d") : RED("Invalid word"), atoi(msg.payload));
                break;
            case MSG_TEMPO_PARTITA:
                *params->timeLeft = atoi(msg.payload);
                snprintf(params->serviceReturnMsg, 100, "In game: %d", *params->timeLeft);
                break;
            case MSG_TEMPO_ATTESA:
                *params->timeLeft = atoi(msg.payload);
                snprintf(params->serviceReturnMsg, 100, "Waiting: %d", *params->timeLeft);
                break;
            default:
                strncpy(params->serviceReturnMsg, msg.payload, 99);
                break;
        }
        pthread_mutex_unlock(&(params->mutex));

        free(buffer);
        displayInterface(params);
    }

    return NULL;
}

void client(int port) {
    struct sockaddr_in server_addr;
    int client_fd, retvalue;
    int score = 0;
    int timeLeft = 0;
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];

    char* clientInput = (char*)malloc(35);
    char* serviceMsg = (char*)malloc(100);

    if (clientInput == NULL || serviceMsg == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");
    SYSC(retvalue, connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "nella connect");

    strcpy(serviceMsg, GREEN("Connected to the server"));
    initMatrix(matrix);

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    ThreadParams params = {
        .clientFd = client_fd,
        .clientInput = clientInput,
        .serviceReturnMsg = serviceMsg,
        .matrix = matrix,
        .score = &score,
        .timeLeft = &timeLeft,
        .mutex = mutex
    };

    pthread_t receiverTid;
    pthread_create(&receiverTid, NULL, handleReceivedMessage, (void *)&params);

    while (1) {
        displayInterface(&params);

        if (scanf("%34[^\n]%*c", clientInput) != 1) {
            printf("Error reading input.\n");
            continue;
        }

        sendMessage((void *)&params);
    }

    free(clientInput);
    free(serviceMsg);
    pthread_mutex_destroy(&mutex);
    SYSC(retvalue, close(client_fd), "nella close");
}

void handleCommand(ThreadParams* params, const char* command, const char* content) {
    int clientFd = params->clientFd;
    int retvalue;

    if (strcmp(command, "aiuto") == 0) {
        strcpy(params->serviceReturnMsg, "Comandi disponibili:\naiuto\nmatrice\np <parola>\nfine\n");
    } else if (strcmp(command, "registra_utente") == 0) {
        Message msg = {.type = MSG_REGISTRA_UTENTE, .size = strlen(content), .payload = content};
        char* buffer;
        int totalSize = serializeMessage(&msg, &buffer);

        SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");
        free(buffer);
    } else if (strcmp(command, "matrice") == 0) {
        Message msg = {.type = MSG_MATRICE, .size = strlen(content), .payload = content};
        char* buffer;
        int totalSize = serializeMessage(&msg, &buffer);

        SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");
        free(buffer);
    } else if (strcmp(command, "p") == 0) {
        Message msg = {.type = MSG_PAROLA, .size = strlen(content), .payload = content};
        char* buffer;
        int totalSize = serializeMessage(&msg, &buffer);

        SYSC(retvalue, write(clientFd, buffer, totalSize), "nella write");
        free(buffer);
    } else if (strcmp(command, "fine") == 0) {
        free(params->clientInput);
        free(params->serviceReturnMsg);
        SYSC(retvalue, close(clientFd), "nella close");
        exit(EXIT_SUCCESS);
    } else {
        strcpy(params->serviceReturnMsg, "Comando non riconosciuto. Digita 'aiuto' per vedere i comandi disponibili.\n");
    }
}

void parseClientInput(const char* clientInput, char* command, char* content) {
    sscanf(clientInput, "%15s%*c%20[^\n]", command, content);
}

void clearScreen() {
    printf("\033[2J"); // Clear screen
    printf("\033[H");  // Move cursor to top-left
}

void displayInterface(const ThreadParams* params) {
    clearScreen();

    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    printf(YELLOW("Time left: %d\n"), *params->timeLeft);
    printMatrix(params->matrix);
    printf(CYAN("Score: %d\n"), *params->score);
    printf("⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻⎻\n");
    printf(MAGENTA("\n%s\n"), params->serviceReturnMsg);
    printf("\n>> ");
    fflush(stdout);
}
