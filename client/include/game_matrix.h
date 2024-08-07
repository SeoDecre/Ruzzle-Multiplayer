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

// Matrix cell struct
typedef struct {
    char letter;
    char color; // 'w' for white, 'b' for black
} Cell;

// Initializes an empty matrix
void initMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);

// Prints the matrix
void printMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);

#endif /* GAME_H */
