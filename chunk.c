//
// Created by chen chen on 2023/10/15.
//

#include "chunk.h"
#include "debug.h"

void initChunk(Chunk *chunk) {
    dbg("Init Chunk")
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk  *chunk) {
    dbg("Free Chunk")
    dbgChunk(chunk, "Chunk Free")
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->size + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->size] = byte;
    chunk->lines[chunk->size] = line;
    chunk->size++;
    dbg("Write [%hhu] To Chunk Line [%d]", byte, line)
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    dbg("Add Value [%g] To Chunk INDEX [%d]", AS_NUMBER(value), chunk->constants.size - 1)
    return chunk->constants.size - 1;
}
