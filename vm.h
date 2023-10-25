//
// Created by chen chen on 2023/10/25.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

/**
 * 虚拟机
 */
typedef struct {
    Chunk* chunk;
    uint8_t* ip; // 记录字节码的位置
} VM;

/**
 * 字节码执行结果
 */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

/**
 * 初始化虚拟机
 */
void initVM();

/**
 * 释放虚拟机
 */
void freeVM();

/**
 * 执行字节码
 * @param chunk
 * @return
 */
InterpretResult interpret(Chunk* chunk);

#endif //CLOX_VM_H
