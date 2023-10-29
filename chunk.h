//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

/**
 * 字节码
 */
typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_RETURN
} OpCode;

/**
 * 指令动态数组
 */
typedef struct {
    int size;
    int capacity;
    uint8_t *code;
    int* lines;
    ValueArray constants;
} Chunk;

/**
 * 初始化动态数组
 * @param chunk
 */
void initChunk(Chunk *chunk);

/**
 * 释放动态数组
 * @param chunk
 */
void freeChunk(Chunk* chunk);

/**
 * 写入动态数组
 * @param chunk
 * @param byte
 */
void writeChunk(Chunk *chunk, uint8_t byte, int line);

/**
 * 添加常量
 * @param chunk
 * @param value
 * @return
 */
int addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
