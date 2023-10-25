//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "memory.h"
#include "value.h"

/**
 * 字节码
 */
typedef enum {
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
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
