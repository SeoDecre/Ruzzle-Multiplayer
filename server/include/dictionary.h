#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game_matrix.h"
#include "macros.h"

#define ALPHABET_SIZE 26
#define AVERAGE_GUESS_WORD_LENGTH 8

// Trie node structure
typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    int isEndOfWord;
} TrieNode;

// Creates a new Trie node
TrieNode* createNode(void);

// Inserts a word into the Trie
void insert(TrieNode *root, char *key);

// Searches for a key in the Trie and returns whether it exists
int search(TrieNode *root, char *key);

// Loads all the words whose length is below the average guessed word length from a file into the Trie
void loadFirstPartOfDictionary(TrieNode *root, char *fileName);

// Loads all the words whose length is above the average guessed word length from a file into the Trie
void loadRestOfDictionary(TrieNode *root, char *fileName);

// Frees the memory allocated for the Trie
void freeTrie(TrieNode *root);

#endif /* DICTIONARY_H */