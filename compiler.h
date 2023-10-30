//
// Created by chen chen on 2023/10/25.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "scanner.h"
#include "object.h"

/**
 * 解析器
 */
typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

/**
 * 局部变量
 */
typedef struct {
    Token name;
    int depth;
    bool isCaptured; // 是否被捕捉
} Local;

/**
 * 上值
 */
typedef struct {
    uint8_t index;
    bool isLocal;
} UpValue;

/**
 * 函数类型
 */
typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

/**
 * 编译器
 */
typedef struct Compiler {
    struct Compiler* enclosing;

    ObjectFunction* function;
    FunctionType type;
    UpValue upValues[UINT8_COUNT];

    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

/**
 * 中缀运算符优先级处理
 */
typedef enum {
    PRECEDENCE_NONE,
    PRECEDENCE_ASSIGNMENT,  // =
    PRECEDENCE_OR,          // or
    PRECEDENCE_AND,         // and
    PRECEDENCE_EQUALITY,    // == !=
    PRECEDENCE_COMPARISON,  // < > <= >=
    PRECEDENCE_TERM,        // + -
    PRECEDENCE_FACTOR,      // * /
    PRECEDENCE_UNARY,       // ! -
    PRECEDENCE_CALL,        // . ()
    PRECEDENCE_PRIMARY
} Precedence;

/**
 * 解析函数
 */
typedef void (*ParseFn)(bool canAssign);

/**
 * 解析表格
 */
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

/**
 * 编译
 * @param source
 * @return
 */
ObjectFunction* compile(const char *source);

/**
 * 标记编译器根
 */
void markCompilerRoots();

#endif //CLOX_COMPILER_H
