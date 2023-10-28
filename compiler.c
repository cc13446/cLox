//
// Created by chen chen on 2023/10/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "debug.h"
#include "object.h"

Parser parser;
Chunk *currentChunk;
Compiler *currentCompiler;

/**
 * 初始化编译器
 * @param compiler
 */
static void initCompiler(Compiler *compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    currentCompiler = compiler;
}

/**
 * 当前字节码块
 * @return
 */
static Chunk *getCurrentChunk() {
    return currentChunk;
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
static void errorAtPrevious(const char *message) {
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

/**
 * 检查当前token是否匹配
 * @param type
 * @return
 */
static bool check(TokenType type) {
    return parser.current.type == type;
}

/**
 * 匹配一个token，并前进一步
 * @param type
 * @return
 */
static bool matchAndNext(TokenType type) {
    if (!check(type)) {
        return false;
    }
    next();
    return true;
}

/**
 * 恐慌恢复
 */
static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:;
                // Do nothing.
        }
        next();
    }
}

// ========================== 字节码输出部分 ==========================

/**
 * 输出字节码
 * @param byte
 */
static void emitByte(uint8_t byte) {
    writeChunk(getCurrentChunk(), byte, parser.previous.line);
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
    int constant = addConstant(getCurrentChunk(), value);
    if (constant > UINT8_MAX) {
        errorAtPrevious("Too many constants in one chunk.");
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
    dbgChunk(getCurrentChunk(), "code");
}

// ========================== 中间代码 ==========================

/**
 * 表达式
 */
static void expression();

/**
 * 数字
 */
static void number(bool canAssign);

/**
 * 字符串
 */
static void string(bool canAssign);

/**
 * 一元表达式
 */
static void unary(bool canAssign);

/**
 * 二元表达式
 */
static void binary(bool canAssign);


/**
 * 括号分组表达式
 */
static void grouping(bool canAssign);

/**
 * 保留字表达式
 */
static void literal(bool canAssign);

/**
 * 变量表达式
 */
static void variable(bool canAssign);

/**
 * 语句
 */
static void statement();

/**
 * 输出语句
 */
static void printStatement();

/**
 * 表达式语句
 */
static void expressionStatement();

/**
 * 块语句
 */
static void block();

/**
 * 声明
 */
static void declaration();

/**
 * 变量声明
 */
static void varDeclaration();

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
        [TOKEN_BANG]          = {unary, NULL, PRECEDENCE_NONE},
        [TOKEN_BANG_EQUAL]    =  {NULL, binary, PRECEDENCE_EQUALITY},
        [TOKEN_EQUAL]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PRECEDENCE_EQUALITY},
        [TOKEN_GREATER]       = {NULL, binary, PRECEDENCE_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PRECEDENCE_COMPARISON},
        [TOKEN_LESS]          = {NULL, binary, PRECEDENCE_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PRECEDENCE_COMPARISON},
        [TOKEN_IDENTIFIER]    = {variable, NULL, PRECEDENCE_NONE},
        [TOKEN_STRING]        = {string, NULL, PRECEDENCE_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PRECEDENCE_NONE},
        [TOKEN_AND]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_CLASS]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PRECEDENCE_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_IF]            = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_NIL]           = {literal, NULL, PRECEDENCE_NONE},
        [TOKEN_OR]            = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_PRINT]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PRECEDENCE_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PRECEDENCE_NONE},
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
        errorAtPrevious("Expect expression.");
        return;
    }

    // 只有当前优先级为 PRECEDENCE_NONE 或者 PRECEDENCE_ASSIGNMENT 才允许附值
    // 这个信息必须传递给变量表达式，避免变量表达式错误的解析了等号，优先级混乱
    bool canAssign = precedence <= PRECEDENCE_ASSIGNMENT;

    // 解析前缀（可能会消耗更多的标识）
    prefixRule(canAssign);

    // 下一个操作符的优先级大于等于自己的时候继续解析中缀
    while (precedence <= getRule(parser.current.type)->precedence) {
        next();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    // 如果变量表达式没有消耗等号，也就是附值给右值的情况，报错
    // 比如是 a * b = 1
    // a 会作为前缀解析
    // * 会作为中缀解析
    // b 会作为变量解析，由于此时为*的优先级，因此不允许附值，等号没被消耗
    // 此时循环遇到当前Token是等号，等号的优先级其实为NONE，是比ASSIGNMENT低的，循环退出
    // 这就达成了canAssign，但是等号没有消耗的场景
    // 应该报错
    // 为什么等号的中缀优先级反而是NONE呢？
    // 因为等号并不是一个中缀运算符，其作为中缀出现的时候，是在附值语句中，不是在表达式中
    // 现在思考一下正确的附值
    // a = 1
    // 这里 a 作为一个前缀解析
    // 变量表达式解析的时候会解析等号
    // 这里附值是成功的
    if (canAssign && matchAndNext(TOKEN_EQUAL)) {
        errorAtPrevious("Invalid assignment target.");
    }
}

/**
 * 生成一个变量对象，将变量加入字节码块常量中，并返回变量在字节码块中的位置
 * @param name
 * @return
 */
static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJECT_VAL(copyString(name->start, name->length)));
}

/**
 * 添加局部变量
 * @param name
 */
static void addLocal(Token name) {
    if (currentCompiler->localCount == UINT8_COUNT) {
        errorAtPrevious("Too many local variables in function.");
        return;
    }
    Local *local = &currentCompiler->locals[currentCompiler->localCount++];
    local->name = name;
    // 这里暂时将变量作用域的深度定义为 -1
    // 避免 var a = a; 这样的语句，因为错误的变量遮蔽找不到变量
    local->depth = -1;
}

/**
 * 检测两个Token 是否相等
 * @param a
 * @param b
 * @return
 */
static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

/**
 * 声明局部变量
 */
static void declareVariable() {
    if (currentCompiler->scopeDepth == 0) {
        return;
    }

    Token *name = &parser.previous;
    // 检测变量重复声明
    for (int i = currentCompiler->localCount - 1; i >= 0; i--) {
        Local *local = &currentCompiler->locals[i];
        // depth为-1代表声明未定义
        if (local->depth != -1 && local->depth < currentCompiler->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            errorAtPrevious("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

/**
 * 解析一个变量名，将变量加入字节码块中，返回变量在字节码块中的位置
 * @param errorMessage
 * @return
 */
static uint8_t parseVariable(const char *errorMessage) {
    consumeAndNext(TOKEN_IDENTIFIER, errorMessage);
    // 如果是局部变量，在这里声明局部变量
    declareVariable();
    // 如果是局部变量，说明变量已经在栈中，返回一个假的表索引，不将其添加到常量表中
    if (currentCompiler->scopeDepth > 0) {
        return 0;
    }
    return identifierConstant(&parser.previous);
}

/**
 * 局部变量声明结束，将深度还原为正确的作用域深度
 */
static void markInitialized() {
    currentCompiler->locals[currentCompiler->localCount - 1].depth = currentCompiler->scopeDepth;
}

/**
 * 定义一个变量
 * @param global
 */
static void defineVariable(uint8_t global) {
    if (currentCompiler->scopeDepth > 0) {
        // 声明变量的时候，变量的深度为-1 这里要将深度还原为正确的作用域深度
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

/**
 * 解析局部变量
 * @param compiler
 * @param name
 * @return
 */
static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            // 变量声明未定义
            // 说明出现了 var a = a;
            // 此时向上级作用域寻找
            if (local->depth != -1) {
                return i;
            }
        }
    }
    return -1;
}

/**
 * 开启一个作用域
 */
static void beginScope() {
    currentCompiler->scopeDepth++;
}

/**
 * 结束一个作用域
 */
static void endScope() {
    currentCompiler->scopeDepth--;
    // 删除死去作用域中的局部变量
    // 这里局部变量在栈中一定是挨在一起的么？
    // 临时变量用过之后都pop了，应该是的
    while (currentCompiler->localCount > 0 &&
           currentCompiler->locals[currentCompiler->localCount - 1].depth > currentCompiler->scopeDepth) {
        emitByte(OP_POP);
        currentCompiler->localCount--;
    }
}

/**
 * 读取一个命名变量
 * @param name
 */
static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    // 先在局部变量中寻找
    int arg = resolveLocal(currentCompiler, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    // canAssign 标志避免变量表达式错误的处理等号
    if (canAssign && matchAndNext(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t) arg);
    } else {
        emitBytes(getOp, (uint8_t) arg);
    }
}

static void expression() {
    parsePrecedence(PRECEDENCE_ASSIGNMENT);
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
    emitConstant(OBJECT_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PRECEDENCE_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        default:
            return; // Unreachable.
    }
}

static void binary(bool canAssign) {
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
        case TOKEN_BANG_EQUAL:
            emitByte(OP_NOT_EQUAL);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        default:
            return; // Unreachable.
    }
}

static void grouping(bool canAssign) {
    expression();
    consumeAndNext(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return; // Unreachable.
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void statement() {
    if (matchAndNext(TOKEN_PRINT)) {
        printStatement();
    } else if (matchAndNext(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

static void printStatement() {
    expression();
    consumeAndNext(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void expressionStatement() {
    expression();
    consumeAndNext(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consumeAndNext(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void declaration() {
    if (matchAndNext(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) {
        synchronize();
    }
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (matchAndNext(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consumeAndNext(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

bool compile(const char *source, Chunk *chunk) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler);

    currentChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;
    next();

    while (!matchAndNext(TOKEN_EOF)) {
        declaration();
    }

    consumeAndNext(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
