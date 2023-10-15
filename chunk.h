//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "memory.h"

/**
 * 字节码
 */
typedef enum {
    OP_RETURN
} OpCode;

/**
 * 指令动态数组
 */
typedef struct {
    int size;
    int capacity;
    uint8_t *code;
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
void writeChunk(Chunk *chunk, uint8_t byte);

#endif //CLOX_CHUNK_H