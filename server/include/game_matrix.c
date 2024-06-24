#include "game_matrix.h"

static char* letters = "ABCDEFGHILMNOP*RSTUVZ";

void initRandomMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    int i, j;

    // Fill the matrix with random letters
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j].letter = letters[rand() % 20];
            matrix[i][j].color = WHITE;
        }
    }
}

void createNextMatrixFromFile(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], const char* fileName) {
    static FILE *file;
    static int currentLine = 0;

    file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[50]; // buffer to hold one line of the file

    // Skip lines until we reach the desired one
    for (int i = 0; i <= currentLine; i++) {
        if (fgets(buffer, sizeof(buffer), file) == NULL) {
            perror("Error reading line from file");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    printf("MATRIX BUFFER: %s\n", buffer);

    // Fill the matrix with characters from the current line
    char* token = strtok(buffer, " ");
    int k = 0;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if (token == NULL) {
                perror("Error parsing line from file");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            matrix[i][j].letter = token[0];
            matrix[i][j].color = WHITE;
            token = strtok(NULL, " ");
            k++;
        }
    }

    // Prepare for the next call
    currentLine++;
    if (feof(file)) {
        fclose(file);
        file = NULL;
        currentLine = 0; // Reset for potential reuse
    }
}

int doesWordExistInMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* word) {
    int found = 0, score = 0, i, j;

    if (strlen(word) < 4 || strlen(word) > 16) return 0;

    // Search for the source cells (first letter of the word)
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            if (TO_LOWERCASE(matrix[i][j].letter) == TO_LOWERCASE(word[0])) {
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
            TO_LOWERCASE(matrix[nextRow][nextCol].letter) == TO_LOWERCASE(word[currentWordIdx + 1]) && matrix[nextRow][nextCol].color == WHITE) {
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
