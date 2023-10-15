//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include <stdio.h>

#include "chunk.h"

#define debug

#ifdef debug
#define dbg(format, ...)                                                            \
    printf("%s %s FILE [%s] LINE [%d] : ", __DATE__, __TIME__, __FILE__, __LINE__); \
    printf(format, ##__VA_ARGS__);                                                  \
    printf("\r\n");
#else
#define dbg(format, ...)
#endif

#ifdef debug
#define dbgChunk(chunk, name) \
    disassembleChunk(chunk, name);
#else
#define dbgChunk(chunk, name)
#endif

/**
 * 反汇编 chunk
 * @param chunk
 * @param name
 */
void disassembleChunk(Chunk* chunk, const char* name);

/**
 * 反汇编 指令
 * @param chunk
 * @param offset
 * @return
 */
int disassembleInstruction(Chunk* chunk, int offset);

#endif //CLOX_DEBUG_H