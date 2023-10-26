//
// Created by chen chen on 2023/10/25.
//
#include <stdio.h>
#include <stdarg.h>

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
 * 运行时错误
 */
static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

static bool isFalse(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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
#define BINARY_OP(valueType, op)                        \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {   \
        runtimeError("Operands must be numbers.");      \
        return INTERPRET_RUNTIME_ERROR;                 \
    }                                                   \
    do {                                                \
        double b = AS_NUMBER(pop());                    \
        double a = AS_NUMBER(pop());                    \
        push(valueType(a op b));                        \
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
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_NOT_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(!valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
                break;
            }
            case OP_ADD: {
                BINARY_OP(NUMBER_VAL, +);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_NOT:
                push(BOOL_VAL(isFalse(pop())));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
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

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);

    dbg("Start Compile")
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    dbg("Success Compile")
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    dbg("Start Run")
    InterpretResult result = run();
    dbg("End Run")

    freeChunk(&chunk);
    return result;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

