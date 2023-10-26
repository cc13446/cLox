//
// Created by chen chen on 2023/10/25.
//

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "debug.h"

Parser parser;
Chunk *compilingChunk;

/**
 * 当前字节码块
 * @return
 */
static Chunk *currentChunk() {
    return compilingChunk;
}

// ========================== AST 解析部分 ==========================

/**
 * 错误报告
 * @param token
 * @param message
 */
static void errorAt(Token *token, const char *message) {
    if (parser.panicMode) {
        return;
    }
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

/**
 * 前一个 Token 有错误
 * @param message
 */
static void errorPrevious(const char *message) {
    errorAt(&parser.previous, message);
}

/**
 * 当前 Token 有错误
 * @param message
 */
static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

/**
 * 前进一步
 */
static void next() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

/**
 * 消耗一个Token，并前进一步
 * @param type
 * @param message
 */
static void consumeAndNext(TokenType type, const char *message) {
    if (parser.current.type == type) {
        next();
        return;
    }
    errorAtCurrent(message);
}

// ========================== 字节码输出部分 ==========================

/**
 * 输出字节码
 * @param byte
 */
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
 * 输出字节码
 * @param byte1
 * @param byte2
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

/**
 * 输出 Return 字节码
 * @param byte
 */
static void emitReturn() {
    emitByte(OP_RETURN);
}

/**
 * 制作 Constant 字节码
 * @param byte
 */
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        errorPrevious("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t) constant;
}

/**
 * 输出 Constant 字节码
 * @param byte
 */
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * 结束编译
 */
static void endCompiler() {
    emitReturn();
    dbgChunk(currentChunk(), "code")
}

// ========================== 中间代码 ==========================

/**
 * 数字
 */
static void number();

/**
 * 一元表达式
 */
static void unary();

/**
 * 二元表达式
 */
static void binary();

/**
 * 表达式
 */
static void expression();

/**
 * 括号分组表达式
 */
static void grouping();

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, NULL, PRECEDENCE_NONE},
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_COMMA]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_DOT]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_MINUS]         = {unary, binary, PRECEDENCE_TERM},
        [TOKEN_PLUS]          = {NULL, binary, PRECEDENCE_TERM},
        [TOKEN_SEMICOLON]     = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_SLASH]         = {NULL, binary, PRECEDENCE_FACTOR},
        [TOKEN_STAR]          = {NULL, binary, PRECEDENCE_FACTOR},
        [TOKEN_BANG]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_EQUAL]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_GREATER]       = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_GREATER_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_LESS]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_LESS_EQUAL]    = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_IDENTIFIER]    = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_STRING]        = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PRECEDENCE_NONE},
        [TOKEN_AND]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_CLASS]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_FALSE]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_IF]            = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_NIL]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_OR]            = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_PRINT]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_TRUE]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PRECEDENCE_NONE},
};

/**
 * 获取规则
 * @param type
 * @return
 */
static ParseRule *getRule(TokenType type) {
    return &rules[type];
}

/**
 * 根据优先级解析表达式
 * @param precedence
 */
static void parsePrecedence(Precedence precedence) {
    next();

    // 根据当前标识查找对应的前缀解析器
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        errorPrevious("Expect expression.");
        return;
    }
    // 解析前缀（可能会消耗更多的标识）
    prefixRule();

    // 下一个操作符的优先级大于等于自己的时候继续解析中缀
    while (precedence <= getRule(parser.current.type)->precedence) {
        next();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PRECEDENCE_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return; // Unreachable.
    }
}

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    // 只有右操作数都比自己多一级的时候才优先计算右操作数
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return; // Unreachable.
    }
}

static void expression() {
    parsePrecedence(PRECEDENCE_ASSIGNMENT);
}

static void grouping() {
    expression();
    consumeAndNext(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

bool compile(const char *source, Chunk *chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    next();

    expression();

    consumeAndNext(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
