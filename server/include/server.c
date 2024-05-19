#include "server.h"
#define RED(string) "\x1b[31m" string "\x1b[0m"
#define GREEN(string) "\x1b[32m" string "\x1b[0m"
#define YELLOW(string) "\x1b[33m" string "\x1b[0m"
#define BLUE(string) "\x1b[34m" string "\x1b[0m"
#define MAGENTA(string) "\x1b[35m" string "\x1b[0m"
#define CYAN(string) "\x1b[36m" string "\x1b[0m"

volatile ServerState currentState = WAITING_STATE;

void switchState(int signum) {
    if (currentState == WAITING_STATE) {
        currentState = GAME_STATE;
        printf(GREEN("Switched to GAME_STATE\n"));
    } else {
        currentState = WAITING_STATE;
        printf(YELLOW("Switched to WAITING_STATE\n"));
    }

    // Set the next alarm for 1 minute
    alarm(30);
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

    // Printing parsed message
    printf("%c %d %s\n", msg.type, msg.size, msg.payload);

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
                    strcpy(responseMsg.payload, "Already registered");
                } else if (nicknameAlreadyExists(params->playersList, msg.payload)) {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "Nickname invalid");
                } else {
                    responseMsg.type = MSG_OK;
                    strcpy(responseMsg.payload, "Game joined");
                    addPlayer(params->playersList, params->player->fd, pthread_self(), msg.payload, 0);
                }
                responseMsg.type == MSG_OK ? printf(GREEN("%s\n"), responseMsg.payload) : printf(RED("%s\n"), responseMsg.payload);
                printPlayerList(params->playersList);
                break;
            case MSG_MATRICE:
                if (isPlayerAlreadyRegistered(params->playersList, params->player->fd)) {
                    responseMsg.type = MSG_MATRICE;
                    memcpy(responseMsg.payload, params->matrix, sizeof(params->matrix));
                } else {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "You're not registered yet");
                }
                break;
            case MSG_PAROLA:
                if (isPlayerAlreadyRegistered(params->playersList, params->player->fd)) {
                    responseMsg.type = MSG_PUNTI_PAROLA;

                    int earnedPoints = 0;                    
                    if (doesWordExistInMatrix(params->matrix, msg.payload)) {
                        earnedPoints = strlen(msg.payload);
                        printf(GREEN("Word %s exists, +%d\n"), msg.payload, strlen(msg.payload));

                        // Updating Player score
                        int updatedScore = getPlayerScore(params->playersList, params->player->fd) + earnedPoints;
                        updatePlayerScore(params->playersList, params->player->fd, updatedScore);
                    }

                    // Sending how much points the given word was as string
                    char pointsString[2];
                    sprintf(pointsString, "%d", earnedPoints);
                    strcpy(responseMsg.payload, pointsString);

                    // Printing players list to see updated score
                    printPlayerList(params->playersList);
                } else {
                    responseMsg.type = MSG_ERR;
                    strcpy(responseMsg.payload, "You're not registered yet");
                }
                break;
            default:
                break;
        }

        // Sending response to client
        responseMsg.size = strlen(responseMsg.payload);
        sendMessageToClient(responseMsg, params->player->fd);

        free(responseMsg.payload);
    }

    printf("Client %s disconnected\n", params->player->name);
    removePlayer(params->playersList, params->player->fd);
    printPlayerList(params->playersList);
    close(params->player->fd);

    // Cleanup thread resources
    pthread_exit(NULL);
}

void server(int port) {
    Cell matrix[MATRIX_SIZE][MATRIX_SIZE];

    // Seed the random number generator with current time
    srand(time(NULL));

    // Filling it with random letters and setting all the cell colors to white
    initMatrix(matrix);

    PlayerList* playersList = createPlayerList();
    int server_fd, retvalue, new_client_fd;
    fd_set readfds;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[MATRIX_BYTES + sizeof(int)];

    // Inizializzazione della struttura server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 127.0.0.1

    // Creazione del socket
    SYSC(server_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Binding
    SYSC(retvalue, bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella bind");

    // Listen
    SYSC(retvalue, listen(server_fd, MAX_CLIENTS), "nella listen");

    printf("Ready to listen.\nGenerated matrix:\n");
    printMatrix(matrix);

    // Set up signal handler and initial alarm
    signal(SIGALRM, switchState);
    alarm(30);

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

        // Creating a new thread to handle the client
        pthread_create(&newPlayer->tid, NULL, handleClient, (void *)params);
    }

    // Chiusura del socket del server (non raggiungibile)
    SYSC(retvalue, close(server_fd), "nella close");
}