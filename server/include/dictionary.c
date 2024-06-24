#include "dictionary.h"

// Function to create a new Trie node
TrieNode* createNode(void) {
    TrieNode *newNode = (TrieNode *)malloc(sizeof(TrieNode));
    if (newNode) {
        newNode->isEndOfWord = false;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            newNode->children[i] = NULL;
        }
    }
    return newNode;
}

// Function to insert a word into the Trie
void insert(TrieNode *root, char *key) {
    TrieNode *crawler = root;
    while (*key) {
        if (*key < 'a' || *key > 'z') {
            // printf("Skipping invalid character: %c\n", *key);
            key++;
            continue;
        }
        int index = *key - 'a';
        if (!crawler->children[index]) {
            // printf("Creating node for character: %c\n", *key);
            crawler->children[index] = createNode();
        }
        crawler = crawler->children[index];
        key++;
    }
    // printf("Marking end of word: %s\n", key);
    crawler->isEndOfWord = true;
}

// Function to search for a word in the Trie
bool search(TrieNode *root, char *key) {
    TrieNode *crawler = root;
    while (*key) {
        if (*key < 'a' || *key > 'z') {
            printf("Invalid character in search: %c\n", *key);
            return false;
        }
        int index = *key - 'a';
        if (!crawler->children[index]) {
            printf("Character not found: %c\n", *key);
            return false;
        }
        crawler = crawler->children[index];
        key++;
    }
    return (crawler != NULL && crawler->isEndOfWord);
}

// Function to load dictionary from file into Trie
void loadDictionary(TrieNode *root, char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open dictionary file");
        return;
    }

    char word[256];
    while (fgets(word, sizeof(word), file)) {
        // Remove newline character
        word[strcspn(word, "\n")] = 0;
        insert(root, word);
    }
    printf("Dictionary loaded\n");
    fclose(file);
}

// Function to free the Trie memory
void freeTrie(TrieNode *root) {
    if (!root) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            freeTrie(root->children[i]);
        }
    }
    free(root);
}
