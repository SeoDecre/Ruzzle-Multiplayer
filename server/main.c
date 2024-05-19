#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "include/macros.h"
#include "include/server.h"

#define DEFAULT_DURATION 3

void usage(const char *progName) {
    fprintf(stderr, "Usage: %s server_name server_port [--matrici dataFilename] [--durata gameDuration] [--seed rndSeed] [--diz dictionary]\n", progName);
    exit(EXIT_FAILURE);
}

void parseArguments(int argc, char *argv[], char **serverName, int *serverPort, char **dataFilename, int *gameDuration, unsigned int *rndSeed, char **dictionary) {
    if (argc < 3) {
        usage(argv[0]);
    }

    *serverName = argv[1];
    *serverPort = atoi(argv[2]);

    if (*serverPort <= 1024) {
        fprintf(stderr, "Port number must be greater than 1024\n");
        exit(EXIT_FAILURE);
    }

    *dataFilename = NULL;
    *gameDuration = DEFAULT_DURATION;
    *rndSeed = 0;
    *dictionary = NULL;

    int opt;
    while (1) {
        static struct option longOptions[] = {
            {"matrici", required_argument, 0, 'm'},
            {"durata", required_argument, 0, 'd'},
            {"seed", required_argument, 0, 's'},
            {"diz", required_argument, 0, 'z'},
            {0, 0, 0, 0}
        };

        int optionIndex = 0;
        opt = getopt_long(argc, argv, "m:d:s:z:", longOptions, &optionIndex);

        if (opt == -1) break;

        switch (opt) {
            case 'm':
                *dataFilename = optarg;
                break;
            case 'd':
                *gameDuration = atoi(optarg);
                if (*gameDuration <= 0) {
                    fprintf(stderr, "Duration must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                *rndSeed = (unsigned int)atoi(optarg);
                break;
            case 'z':
                *dictionary = optarg;
                break;
            default:
                usage(argv[0]);
        }
    }
}

void printConfig(const char *serverName, int serverPort, const char *dataFilename, int gameDuration, unsigned int rndSeed, const char *dictionary) {
    printf("Server name: %s\n", serverName);
    printf("Server port: %d\n", serverPort);
    dataFilename ? printf("Data filename: %s\n", dataFilename) : printf("Data filename: not provided, will generate matrices randomly.\n");

    printf("Game duration: %d minutes\n", gameDuration);
    printf("Random seed: %u\n", rndSeed);
    dictionary ? printf("Dictionary file: %s\n", dictionary) :printf("Dictionary file: not provided, using default dictionary.\n");
}

int main(int argc, char *argv[]) {
    char *serverName;
    int serverPort;
    char *dataFilename;
    int gameDuration;
    unsigned int rndSeed;
    char *dictionary;

    parseArguments(argc, argv, &serverName, &serverPort, &dataFilename, &gameDuration, &rndSeed, &dictionary);
    printConfig(serverName, serverPort, dataFilename, gameDuration, rndSeed, dictionary);

    // Your server initialization and execution code goes here
    printf("Server started at localhost:%d\n", serverPort);
    server(serverPort);

    return 0;
}
