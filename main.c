//
// Created by chen chen on 2023/10/15.
//
#include "common.h"
#include "chunk.h"

int main(int argc, char* argv[]) {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);
    freeChunk(&chunk);
    return 0;
}
