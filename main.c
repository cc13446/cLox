//
// Created by chen chen on 2023/10/15.
//
#include "common.h"
#include "chunk.h"
#include "debug.h"
int main(int argc, char* argv[]) {
    Chunk chunk;
    initChunk(&chunk);
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT);
    writeChunk(&chunk, constant);
    dbgChunk(&chunk, "Add Constant")
    writeChunk(&chunk, OP_RETURN);
    dbgChunk(&chunk, "Add Return")
    freeChunk(&chunk);
    return 0;
}
