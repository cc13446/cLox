//
// Created by chen chen on 2023/10/25.
//

#ifndef CLOX_TRIE_H
#define CLOX_TRIE_H

#include "scanner.h"

typedef struct TrieNode {
    struct TrieNode *node;
    TokenType type;
} Trie;

/**
 * 初始化 key word trie
 */
void initKeyWordTrie();

/**
 * 添加关键字
 * @param word
 * @param type
 */
void addKeyWord(const char *word, TokenType type);

/**
 * 匹配关键字
 * @param word
 * @param len
 * @return
 */
TokenType matchTokenWord(const char *word, long len);

/**
 * 释放Trie
 */
void freeTrie();

#endif //CLOX_TRIE_H
