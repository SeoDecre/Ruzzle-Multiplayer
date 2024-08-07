#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dictionary.h"
#include "colors.h"
#include "macros.h"

#define MATRIX_SIZE 4
#define MATRIX_BYTES (MATRIX_SIZE * MATRIX_SIZE * sizeof(Cell))
#define MAX_WORD_LENGTH 16
#define WHITE 'w'
#define BLACK 'b'

// Matrix cell structure
typedef struct {
    char letter;
    char color; // 'w' for white, 'b' for black
} Cell;


// Initializes the matrix with random values
void initRandomMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);

// Initializes the matrix using the next line of the passed file
void createNextMatrixFromFile(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* fileName);

// Replaces occurrences of "qu" in a word with a single 'q' character
void replaceQu(char* word);

// Checks if a given word exists in the matrix
int doesWordExistInMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* word);

// Validates if the current word can be formed starting from a specific position in the matrix
void isWordValid(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], int* found, char* word, int currentWordIdx, int currentRow, int currentCol);

// Cleans the matrix by resetting its values
void cleanMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);

// Prints the contents of the matrix
void printMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]);

#endif /* GAME_H */
