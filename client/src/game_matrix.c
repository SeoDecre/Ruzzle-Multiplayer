#include "game_matrix.h"

void initMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    int i, j;

    // Fill the matrix with random letters
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j].letter = '-';
            matrix[i][j].color = WHITE;
        }
    }
}

void cleanMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j].color = WHITE;
        }
    }
}

void printMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    if (matrix[0][0].letter == '-') return;
    int i, j;
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            printf("%c ", matrix[i][j].letter);
        }
        printf("\n");
    }
}
