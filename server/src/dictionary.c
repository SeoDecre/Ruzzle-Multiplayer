#include "dictionary.h"

TrieNode* createNode(void) {
    // TrieNode *newNode = (TrieNode *)malloc(sizeof(TrieNode));
    TrieNode *newNode;
    ALLOCATE_MEMORY(newNode, sizeof(TrieNode), "Unable to allocate memory for dictionary trie\n");
    if (newNode) {
        newNode->isEndOfWord = 0;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            newNode->children[i] = NULL;
        }
    }
    return newNode;
}

void insert(TrieNode *root, char *key) {
    TrieNode *crawler = root;

    // Replacing Q with Qu because that's how words are stored in the trie
    replaceQu(key);

    while (*key) {
        // Putting key to lowercase if necessary
        // This condition sligthly slows down the loadDictionary function if the file is too big
        // But it's not a big deal since the 
        if (*key >= 'A' && *key <= 'Z') *key += ('a' - 'A');

        // Skipping invalid characters
        if (*key < 'a' || *key > 'z') {
            key++;
            continue;
        }

        int index = *key - 'a';
        if (!crawler->children[index]) {
            // Creating node for new character
            crawler->children[index] = createNode();
        }
        crawler = crawler->children[index];
        key++;
    }

    // Marking end of word
    crawler->isEndOfWord = 1;
}

int search(TrieNode *root, char *key) {
    TrieNode *crawler = root;
    while (*key) {
        // Invalid character in search
        if (*key < 'a' || *key > 'z') return 0;

        // Checking if current character exists in the structure
        int index = *key - 'a';
        if (!crawler->children[index]) return 0;
        crawler = crawler->children[index];
        key++;
    }
    return (crawler != NULL && crawler->isEndOfWord);
}

void loadFirstPartOfDictionary(TrieNode *root, char *fileName) {
    FILE *file;
    FILE_OPEN(file, fileName, "r");

    char word[256];
    while (fgets(word, sizeof(word), file)) {
        // Insert word in the trie just if its length is below the average guessed word length
        if (strlen(word) <= AVERAGE_GUESS_WORD_LENGTH) {
            // Remove newline character
            word[strcspn(word, "\n")] = 0;
            insert(root, word);
        }
    }

    printf("Dictionary loaded\n");
    fclose(file);
}

void loadRestOfDictionary(TrieNode *root, char *fileName) {
    FILE *file;
    FILE_OPEN(file, fileName, "r");

    char word[256];
    while (fgets(word, sizeof(word), file)) {
        // Insert all the words above the average length but below the possible maximum given the matrix size
        if (strlen(word) > AVERAGE_GUESS_WORD_LENGTH && strlen(word) <= (MATRIX_SIZE*MATRIX_SIZE)) {
            // Remove newline character
            word[strcspn(word, "\n")] = 0;
            insert(root, word);
        }
    }

    printf("Dictionary loaded\n");
    fclose(file);
}

void freeTrie(TrieNode *root) {
    if (!root) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            freeTrie(root->children[i]);
        }
    }
    free(root);
}
