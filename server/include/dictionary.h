#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "macros.h"

#define ALPHABET_SIZE 26

// Trie node structure
typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool isEndOfWord;
} TrieNode;

// Function prototypes
TrieNode* createNode(void);
void insert(TrieNode *root, char *key);
bool search(TrieNode *root, char *key);
void loadDictionary(TrieNode *root, char *filename);
void freeTrie(TrieNode *root);

#endif /* DICTIONARY_H */