//
// Created by chen chen on 2023/10/25.
//
#include <stdlib.h>
#include <stdio.h>

#include "trie.h"
#include "common.h"

#define TRIE_MAX_LENGTH 26

Trie trie;

void initKeyWordTrie() {
    trie.node = NULL;
    trie.type = TOKEN_ERROR;
    addKeyWord("and", TOKEN_AND);
    addKeyWord("class", TOKEN_CLASS);
    addKeyWord("else", TOKEN_ELSE);
    addKeyWord("false", TOKEN_FALSE);
    addKeyWord("for", TOKEN_FOR);
    addKeyWord("fun", TOKEN_FUN);
    addKeyWord("if", TOKEN_IF);
    addKeyWord("nil", TOKEN_NIL);
    addKeyWord("or", TOKEN_OR);
    addKeyWord("print", TOKEN_PRINT);
    addKeyWord("return", TOKEN_RETURN);
    addKeyWord("super", TOKEN_SUPER);
    addKeyWord("this", TOKEN_THIS);
    addKeyWord("true", TOKEN_TRUE);
    addKeyWord("var", TOKEN_VAR);
    addKeyWord("while", TOKEN_WHILE);
}

void addKeyWord(const char *word, TokenType type) {
    const char *cur = word;
    Trie *t = &trie;
    while (*cur != '\0') {
        if (t->node == NULL) {
            t->node = (Trie *) malloc(sizeof(Trie) * TRIE_MAX_LENGTH);
            Trie *n = t->node;
            for (int i = 0; i < TRIE_MAX_LENGTH; i++, n++) {
                n->node = NULL;
                n->type = TOKEN_ERROR;
            }
        }
        int index = *cur - 'a';
        if (index < 0 || index >= TRIE_MAX_LENGTH) {
            fprintf(stderr, "Invalid key word \"%s\".\n", word);
            exit(73);
        }
        t = &t->node[index];
        cur++;
    }
    t->type = type;
}

TokenType matchTokenWord(const char *word, long len) {
    const char *cur = word;
    Trie *t = &trie;
    for (int i = 0; i < len; i++, cur++) {
        if (t == NULL || t->node == NULL) {
            return TOKEN_ERROR;
        }
        int index = *cur - 'a';
        if (index < 0 || index >= TRIE_MAX_LENGTH) {
            return TOKEN_ERROR;
        }
        t = &t->node[index];
    }
    return t->type;
}

static void doFreeTrie(Trie *t) {
    if (t == NULL || t->node == NULL) {
        return;
    }
    Trie *node = t->node;
    for (int i = 0; i < TRIE_MAX_LENGTH; i++, node++) {
        doFreeTrie(node);
    }
    free(t->node);
}

void freeTrie() {
    doFreeTrie(&trie);
}
