#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MATRIX_SIZE 4
#define MATRIX_BYTES (MATRIX_SIZE * MATRIX_SIZE * sizeof(Cell))
#define MAX_WORD_LENGTH 16
#define WHITE 'w'
#define BLACK 'b'

typedef struct {
    char letter;
    char color; // 'w' for white, 'b' for black
} Cell;

// Function prototypes
void initMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);
void cleanMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);
void printMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);

#endif /* GAME_H */
