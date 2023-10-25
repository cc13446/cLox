//
// Created by chen chen on 2023/10/15.
//
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char* argv[]) {
    initVM();
    Chunk chunk;
    initChunk(&chunk);
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 1);
    writeChunk(&chunk, constant, 1);
    dbgChunk(&chunk, "Add Constant")
    writeChunk(&chunk, OP_RETURN, 1);
    dbgChunk(&chunk, "Add Return")
    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);
    return 0;
}
