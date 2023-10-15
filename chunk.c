//
// Created by chen chen on 2023/10/15.
//

#include <stdlib.h>
#include "chunk.h"
#include "debug.h"

void initChunk(Chunk *chunk) {
    dbg("Init Chunk")
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->code = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk  *chunk) {
    dbg("Free Chunk")
    dbgChunk(chunk, "Chunk Free")
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte) {
    if (chunk->capacity < chunk->size + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->size] = byte;
    chunk->size++;
    dbg("Write [%hhu] To Chunk ", byte)
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    dbg("Add Value [%g] To Chunk INDEX [%d]", value, chunk->constants.size - 1)
    return chunk->constants.size - 1;
}
