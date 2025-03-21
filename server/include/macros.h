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
#define CHECK_NULL(ptr, m) if ((ptr) == NULL) { perror(m); exit(errno); }

// Check if content is negative and handle error
#define CHECK_NEGATIVE(value, m) if ((value) < 0) { perror(m); exit(errno); }

// Check file opening and handle error
#define FILE_OPEN(fp, fname, mode) if ((fp = fopen(fname, mode)) == NULL) { perror("Error opening file"); exit(errno); }

// To lower-case character
#define TO_LOWERCASE(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))

// Malloc which also checks for errors
// It uses a single-cycled do-while so that the macro is treated as a regular statement
#define ALLOCATE_MEMORY(ptr, size, m) \
    do { \
        ptr = malloc(size); \
        if (ptr == NULL) { \
            perror(m); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

static const int dummy = 0;

#endif /* MACROS_H */
