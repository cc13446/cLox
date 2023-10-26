//
// Created by chen chen on 2023/10/25.
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"

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

/**
 * 判断是否为 False
 * @param value
 * @return
 */
static bool isFalse(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/**
 * 合并字符串
 */
static void concatString() {
    ObjectString *b = AS_STRING(pop());
    ObjectString *a = AS_STRING(pop());

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjectString *result = takeString(chars, length);
    push(OBJECT_VAL(result));
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
        dbgStack(vm);
        dbgInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code));
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
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatString();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
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

/**
 * 释放对象
 */
static void freeObject(Object *object) {
    switch (object->type) {
        case OBJECT_STRING: {
            ObjectString *string = (ObjectString *) object;
            dbg("Free Memory of Object String %s", string->chars);
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjectString, object);
            break;
        }
    }
}

/**
 * 释放所有对象
 */
static void freeObjects() {
    Object *object = vm.objects;
    while (object != NULL) {
        Object *next = object->next;
        freeObject(object);
        object = next;
    }
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
}

void freeVM() {
    freeTable(&vm.strings);
    freeObjects();
}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);

    dbg("Start Compile");
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    dbg("Success Compile");
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    dbg("Start Run");
    InterpretResult result = run();
    dbg("End Run");

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

void addObject(Object *object) {
    object->next = vm.objects;
    vm.objects = object;
}

void holdString(ObjectString *string) {
    tableSet(&vm.strings, string, NIL_VAL);
}

ObjectString *findSting(const char *chars, int length, u_int32_t hash) {
    return tableFindKey(&vm.strings, chars, length, hash);
}

