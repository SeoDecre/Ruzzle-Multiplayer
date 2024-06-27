#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "macros.h"
#include "server.h"

#define DEFAULT_DURATION 180

void usage(char *progName) {
    fprintf(stderr, "Usage: %s server_name server_port [--matrici matrixFilename] [--durata gameDuration] [--seed rndSeed] [--diz newDictionaryFile]\n", progName);
    exit(EXIT_FAILURE);
}

void parseArguments(int argc, char *argv[], char **serverName, int *serverPort, char **matrixFilename, int *gameDuration, unsigned int *rndSeed, char **newDictionaryFile) {
    if (argc < 3) {
        usage(argv[0]);
    }

    *serverName = argv[1];
    *serverPort = atoi(argv[2]);

    if (*serverPort <= 1024) {
        fprintf(stderr, "Port number must be greater than 1024\n");
        exit(EXIT_FAILURE);
    }

    *matrixFilename = NULL;
    *gameDuration = DEFAULT_DURATION;
    *rndSeed = 0;
    *newDictionaryFile = NULL;

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
                *matrixFilename = optarg;
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
                *newDictionaryFile = optarg;
                break;
            default:
                usage(argv[0]);
        }
    }
}

void printConfig(char *serverName, int serverPort, char *matrixFilename, int gameDuration, unsigned int rndSeed, char *newDictionaryFile) {
    printf("Server name: %s\n", serverName);
    printf("Server port: %d\n", serverPort);
    matrixFilename ? printf("Matrix filename: %s\n", matrixFilename) : printf("Matrix filename: not provided, will generate matrices randomly.\n");

    printf("Game duration: %d seconds\n", gameDuration);
    printf("Random seed: %u\n", rndSeed);
    newDictionaryFile ? printf("New dictionary file: %s\n", newDictionaryFile) :printf("New dictionary file: not provided, using default newDictionaryFile\n");
}

int main(int argc, char *argv[]) {
    char *serverName;
    int serverPort;
    char *matrixFilename;
    int gameDuration;
    unsigned int rndSeed;
    char *newDictionaryFile;

    parseArguments(argc, argv, &serverName, &serverPort, &matrixFilename, &gameDuration, &rndSeed, &newDictionaryFile);
    printConfig(serverName, serverPort, matrixFilename, gameDuration, rndSeed, newDictionaryFile);

    // Server initialization
    server(serverName, serverPort, matrixFilename, gameDuration, rndSeed, newDictionaryFile);

    return 0;
}
