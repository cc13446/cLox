//
// Created by chen chen on 2023/10/25.
//

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/**
 * 单例
 */
Scanner scanner;

/**
 * 是否结束
 * @return
 */
static bool isAtEnd() {
    return *scanner.current == '\0';
}

/**
 * 制作一个Token
 */
static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

/**
 * 错误Token
 * @param message
 * @return
 */
static Token makeErrorToken(const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner.line;
    return token;
}

/**
 * 返回当前Char，并前进一步
 * @return
 */
static char getCurrentCharAndNext() {
    scanner.current++;
    return scanner.current[-1];
}

/**
 * 返回当前Char
 * @return
 */
static char peekCurrentChar() {
    return *scanner.current;
}

/**
 * 返回下一个Char
 * @return
 */
static char peekNextChar() {
    if (isAtEnd()) {
        return '\0';
    }
    return scanner.current[1];
}

/**
 * 匹配当前Char，如果匹配到了则前进一步
 * @param expected
 * @return
 */
static bool matchCurrentCharAndNext(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

/**
 * 跳过空白字符
 */
static void skipWhitespace() {
    for (;;) {
        char c = peekCurrentChar();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                getCurrentCharAndNext();
                break;
            case '\n':
                scanner.line++;
                getCurrentCharAndNext();
                break;
            case '/':
                if (peekNextChar() == '/') {
                    // A comment goes until the end of the line.
                    while (peekCurrentChar() != '\n' && !isAtEnd()) {
                        getCurrentCharAndNext();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

void initScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = getCurrentCharAndNext();

    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(matchCurrentCharAndNext('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(matchCurrentCharAndNext('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(matchCurrentCharAndNext('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(matchCurrentCharAndNext('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return makeErrorToken("Unexpected character.");
}