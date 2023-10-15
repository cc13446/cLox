//
// Created by chen chen on 2023/10/15.
//

#include <stdlib.h>
#include "chunk.h"
#include "dbg.h"

void initChunk(Chunk *chunk) {
    dbg("Init Chunk")
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->code = NULL;
}

void freeChunk(Chunk  *chunk) {
    dbg("Free Chunk %s", chunk->code)
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte) {
    dbg("Write Chunk %hhu to %s", byte, chunk->code)
    if (chunk->capacity < chunk->size + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->size] = byte;
    chunk->size++;
}


