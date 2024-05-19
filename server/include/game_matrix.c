#include "game_matrix.h"

static char* letters = "ABCDEFGHILMNOPQRSTUVZ";

void initMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    int i, j;

    // Fill the matrix with random letters
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j].letter = letters[rand() % 20];
            matrix[i][j].color = WHITE;
        }
    }
}

int doesWordExistInMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* word) {
    int found = 0, score = 0, i, j;

    if (strlen(word) < 4 || strlen(word) > 16) return 0;

    // Search for the source cells (first letter of the word)
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            if (matrix[i][j].letter == word[0]) {
                // Perform DFS to check for the word
                isWordValid(matrix, &found, word, 0, i, j);
                // Clean the matrix for next searches
                cleanMatrix(matrix);
                // The function isWordValid correctly updated the found value
                if (found) return found;
            }
        }
    }

    // If the loop hasn't been stopped then the found value is still 0
    return found;
}

void isWordValid(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], int* found, char* word, int currentWordIdx, int currentRow, int currentCol) {
    if (currentWordIdx >= strlen(word) - 1) {
        *found = 1;
        return;
    }

    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // Up, Down, Left, Right

    for (int d = 0; d < 4; ++d) {
        int nextRow = currentRow + directions[d][0];
        int nextCol = currentCol + directions[d][1];

        // Avoid useless backtracking recursive calls
        if (nextRow >= 0 && nextRow < MATRIX_SIZE && nextCol >= 0 && nextCol < MATRIX_SIZE &&
            matrix[nextRow][nextCol].letter == word[currentWordIdx + 1] && matrix[nextRow][nextCol].color == WHITE) {
            matrix[currentRow][currentCol].color = BLACK;
            isWordValid(matrix, found, word, currentWordIdx + 1, nextRow, nextCol);
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
    int i, j;
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            printf("%c(%c) ", matrix[i][j].letter, matrix[i][j].color);
        }
        printf("\n");
    }
}
