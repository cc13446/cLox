//
// Created by chen chen on 2023/10/25.
//
#include <stdio.h>

#include "common.h"
#include "vm.h"
#include "debug.h"

/**
 * 单例
 */
VM vm;

void initVM() {
}

void freeVM() {
}

/**
 * 执行字节码
 * @return
 */
static InterpretResult run() {
// 读取字节码
#define READ_BYTE() (*vm.ip++)
// 读取常量
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    for (;;) {
        dbgInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code))
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                printValue(constant);
                printf("\n");
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(Chunk *chunk) {
    dbg("Start Run")
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    InterpretResult result = run();
    dbg("End Run")
    return result;
}

