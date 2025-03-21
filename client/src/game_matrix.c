#include "game_matrix.h"

void initMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    int i, j;

    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j].letter = '-';
            matrix[i][j].color = WHITE;
        }
    }
}

void printMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    if (matrix[0][0].letter == '-') return;
    int i, j;
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j].letter == 'Q' ? printf("Qu ") : printf("%c ", matrix[i][j].letter);
        }
        printf("\n");
    }
}
