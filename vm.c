//
// Created by chen chen on 2023/10/25.
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

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
    vm.frameCount = 0;
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

    // 打印栈帧
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjectFunction *function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();
}

/**
 * 定义本地函数
 * @param name
 * @param function
 */
static void defineNative(const char *name, NativeFn function) {
    push(OBJECT_VAL(copyString(name, (int) strlen(name))));
    push(OBJECT_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

/**
 * 添加时钟函数
 * @param argCount
 * @param args
 * @return
 */
static Value clockNative(int argCount, Value *args) {
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
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
 * 函数调用
 * @param function
 * @param argCount
 * @return
 */
static bool call(ObjectFunction *function, int argCount) {
    // 参数数量检查
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    }

    // 调用溢出检查
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    // 复用形参
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

/**
 * 函数调用检查
 * @param callee
 * @param argCount
 * @return
 */
static bool callValue(Value callee, int argCount) {
    if (IS_OBJECT(callee)) {
        switch (OBJECT_TYPE(callee)) {
            case OBJECT_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            case OBJECT_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

/**
 * 执行字节码
 * @return
 */
static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

// 读取字节码
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
// 读取常量
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
// 读取字符串
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
        dbgInstruction(&frame->function->chunk, (int) (frame->ip - frame->function->chunk.code));
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                Value result = pop();
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
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
            case OP_POP:
                pop();
                break;
            case OP_DEFINE_GLOBAL: {
                ObjectString *name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjectString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjectString *name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
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
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalse(peek(0))) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                // 函数调用会创建新的栈侦
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
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
        case OBJECT_FUNCTION: {
            ObjectFunction *function = (ObjectFunction *) object;
            freeChunk(&function->chunk);
            FREE(ObjectFunction, object);
            break;
        }
        case OBJECT_NATIVE:
            FREE(ObjectNative, object);
            break;
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
    initTable(&vm.globals);
    defineNative("clock", clockNative);
}

void freeVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

InterpretResult interpret(const char *source) {
    dbg("Start Compile");
    ObjectFunction *function = compile(source);
    dbg("Success Compile");
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(OBJECT_VAL(function));
    call(function, 0);

    dbg("Start Run");
    InterpretResult result = run();
    dbg("End Run");

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

