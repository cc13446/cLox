//
// Created by chen chen on 2023/10/25.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#define STACK_MAX 256

#include "chunk.h"
#include "value.h"

/**
 * 虚拟机
 */
typedef struct {
    Chunk *chunk;
    uint8_t *ip; // 记录字节码的位置
    Value stack[STACK_MAX]; // 虚拟机栈
    Value *stackTop; // 虚拟机栈顶
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
InterpretResult interpret(Chunk *chunk);

/**
 * 入栈
 * @param value
 */
void push(Value value);

/**
 * 栈顶元素出栈
 * @return
 */
Value pop();

#endif //CLOX_VM_H
