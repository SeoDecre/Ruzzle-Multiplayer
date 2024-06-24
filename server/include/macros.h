#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// Check system call return value for -1 and handle error
#define SYSC(v, c, m) if ((v = c) == -1) { perror(m); exit(errno); }

// Check system call return value for NULL and handle error
#define SYSCN(v, c, m) if ((v = c) == NULL) { perror(m); exit(errno); }

// Check if pointer is NULL and handle error
#define CHECK_NULL(ptr, m) if ((ptr) == NULL) { fprintf(stderr, "%s: %s\n", m, strerror(errno)); exit(errno); }

// Check condition and handle error if true
#define CHECK_COND(cond, m) if (cond) { fprintf(stderr, "%s: %s\n", m, strerror(errno)); exit(errno); }

// Check file opening and handle error
#define FILE_OPEN(fp, fname, mode) if ((fp = fopen(fname, mode)) == NULL) { perror("Error opening file"); exit(errno); }

// Custom error message and exit
#define ERROR_MSG(m) { fprintf(stderr, "%s\n", m); exit(EXIT_FAILURE); }

// Print error message with additional info and exit
#define ERROR_MSG_INFO(m, info) { fprintf(stderr, "%s: %s\n", m, info); exit(EXIT_FAILURE); }

// To lower-case character
#define TO_LOWERCASE(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))

static const int dummy = 0;

#endif /* MACROS_H */
