//
// Created by chen chen on 2023/10/25.
//
#include <stdio.h>

#include "vm.h"
#include "debug.h"
#include "compiler.h"

/**
 * 单例
 */
VM vm;

/**
 * 重制虚拟机栈顶
 */
static void resetStack() {
    vm.stackTop = vm.stack;
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
// 二元运算
#define BINARY_OP(op)       \
    do {                    \
        double b = pop();   \
        double a = pop();   \
        push(a op b);       \
    } while (false)

    for (;;) {
        dbgStack(vm)
        dbgInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code))
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD: {
                BINARY_OP(+);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(/);
                break;
            }
            case OP_NEGATE: {
                push(-pop());
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

void initVM() {
    resetStack();
}

void freeVM() {
}

InterpretResult interpret(const char* source) {
    dbg("Start Compile")
    compile(source);
    dbg("End Compile")
    return INTERPRET_OK;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

