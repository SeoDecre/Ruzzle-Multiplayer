#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "include/macros.h"
#include "include/client.h"

int main(int argc, char *argv[]) {
    int port = atoi(argv[1]);
    client(port);

    return 0;
}
