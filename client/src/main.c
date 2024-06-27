#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* serverName = argv[1];
    int port = atoi(argv[2]);

    client(serverName, port);

    return 0;
}
