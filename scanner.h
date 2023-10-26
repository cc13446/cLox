//
// Created by chen chen on 2023/10/25.
//

#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

typedef enum {
    // Single-character tokens. 单字符词法
    TOKEN_LEFT_PAREN,   // 0
    TOKEN_RIGHT_PAREN,  // 1
    TOKEN_LEFT_BRACE,   // 2
    TOKEN_RIGHT_BRACE,  // 3
    TOKEN_COMMA,        // 4
    TOKEN_DOT,          // 5
    TOKEN_MINUS,        // 6
    TOKEN_PLUS,         // 7
    TOKEN_SEMICOLON,    // 8
    TOKEN_SLASH,        // 9
    TOKEN_STAR,         // 10

    // One or two character tokens. 一或两字符词法
    TOKEN_BANG,         // 11
    TOKEN_BANG_EQUAL,   // 12
    TOKEN_EQUAL,        // 13
    TOKEN_EQUAL_EQUAL,  // 14
    TOKEN_GREATER,      // 15
    TOKEN_GREATER_EQUAL,// 16
    TOKEN_LESS,         // 17
    TOKEN_LESS_EQUAL,   // 18

    // Literals. 字面量
    TOKEN_IDENTIFIER,   // 19
    TOKEN_STRING,       // 20
    TOKEN_NUMBER,       // 21

    // Keywords. 关键字
    TOKEN_AND,          // 22
    TOKEN_CLASS,        // 23
    TOKEN_ELSE,         // 24
    TOKEN_FALSE,        // 25
    TOKEN_FOR,          // 26
    TOKEN_FUN,          // 27
    TOKEN_IF,           // 28
    TOKEN_NIL,          // 29
    TOKEN_OR,           // 30
    TOKEN_PRINT,        // 31
    TOKEN_RETURN,       // 32
    TOKEN_SUPER,        // 33
    TOKEN_THIS,         // 34
    TOKEN_TRUE,         // 35
    TOKEN_VAR,          // 36
    TOKEN_WHILE,        // 37

    TOKEN_ERROR,        // 38
    TOKEN_EOF           // 39
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;

/**
 * 扫描器
 */
typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

/**
 * 初始化扫描器
 * @param source
 */
void initScanner(const char *source);

/**
 * 扫描一个Token
 * @return
 */
Token scanToken();

#endif //CLOX_SCANNER_H
