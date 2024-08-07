#include "game_matrix.h"

static char* letters = "ABCDEFGHILMNOPQRSTUVZ";

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

void createNextMatrixFromFile(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* fileName) {
    // This variables are static so that they are preserved between each call
    static FILE *file;
    static int currentLine = 0;
    FILE_OPEN(file, fileName, "r");

    char buffer[50]; // buffer to hold one line of the file

    // Skip lines until we reach the desired one
    for (int i = 0; i <= currentLine; i++) {
        if (fgets(buffer, sizeof(buffer), file) == NULL) {
            perror("Error reading line from file");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

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

void replaceQu(char* word) {
    // Edge case: empty string or NULL pointer
    if (word == NULL || *word == '\0') return;

    int len = strlen(word);
    for (int i = 0; i < len - 1; ++i) {
        if (word[i] == 'q' && word[i + 1] == 'u') {
            // Shift characters left to remove 'u'
            for (int j = i + 1; j < len - 1; ++j) word[j] = word[j + 1];
            word[len - 1] = '\0';  // Null-terminate the string
            len--;  // Decrease length of string
            i--;  // Check the current position again after shifting
        }
    }
}

int doesWordExistInMatrix(Cell matrix[MATRIX_SIZE][MATRIX_SIZE], char* word) {    
    // Additionally, we could add a condition to check if the word contains letters present in the matrix
    // However, considering the matrix size will remain relatively small for this type of game, iterating through each letter of the word might be inefficient in most cases
    if (strlen(word) < 4 || strlen(word) > (MATRIX_SIZE*MATRIX_SIZE)) return 0;

    int found = 0, i, j;

    // Search for the source cells (first letter of the word)
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            if (TO_LOWERCASE(matrix[i][j].letter) == word[0]) {
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
    if (currentWordIdx >= (int)strlen(word) - 1) {
        *found = 1;
        return;
    }

    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // Up, Down, Left, Right

    for (int d = 0; d < 4; ++d) {
        int nextRow = currentRow + directions[d][0];
        int nextCol = currentCol + directions[d][1];

        // Avoid useless backtracking recursive calls
        if (nextRow >= 0 && nextRow < MATRIX_SIZE && nextCol >= 0 && nextCol < MATRIX_SIZE &&
            TO_LOWERCASE(matrix[nextRow][nextCol].letter) == word[currentWordIdx + 1] && matrix[nextRow][nextCol].color == WHITE) {
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
            matrix[i][j].letter == 'Q' ? 
                printf(MAGENTA("Qu ")) :
                printf(MAGENTA("%c "), matrix[i][j].letter);
        }
        printf("\n");
    }
}
