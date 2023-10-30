//
// Created by chen chen on 2023/10/25.
//
#include <stdio.h>
#include <stdlib.h>
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
    vm.openUpValues = NULL;
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
        ObjectFunction *function = frame->closure->function;
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
    ObjectString *b = AS_STRING(peek(0));
    ObjectString *a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjectString *result = takeString(chars, length);

    pop();
    pop();
    push(OBJECT_VAL(result));
}

/**
 * 函数调用
 * @param function
 * @param argCount
 * @return
 */
static bool call(ObjectClosure *closure, int argCount) {
    // 参数数量检查
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    // 调用溢出检查
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
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
            case OBJECT_BOUND_METHOD: {
                ObjectBoundMethod *bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            case OBJECT_CLASS: {
                ObjectClass *klass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJECT_VAL(newInstance(klass));

                // 初始化函数
                Value initializer;
                if (tableGet(&klass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }

                return true;
            }
            case OBJECT_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
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
 * 捕获上值
 * @param local
 * @return
 */
static ObjectUpValue *captureUpValue(Value *local) {
    // 所有关闭的上值都会记录在这个链表中
    // 因此捕获上值之前，先看看这个表中有没有
    ObjectUpValue *prevUpValue = NULL;
    ObjectUpValue *upValue = vm.openUpValues;
    while (upValue != NULL && upValue->location > local) {
        prevUpValue = upValue;
        upValue = upValue->next;
    }

    if (upValue != NULL && upValue->location == local) {
        return upValue;
    }

    ObjectUpValue *createdUpValue = newUpValue(local);
    createdUpValue->next = upValue;
    // 如果在上表中没有查到，这里捕获了新的上值，在表中记录一下
    if (prevUpValue == NULL) {
        vm.openUpValues = createdUpValue;
    } else {
        prevUpValue->next = createdUpValue;
    }
    return createdUpValue;
}

/**
 * 关闭上值
 * @param last
 */
static void closeUpValues(Value *last) {
    while (vm.openUpValues != NULL && vm.openUpValues->location >= last) {
        ObjectUpValue *upValue = vm.openUpValues;
        // 隐含的拷贝
        upValue->closed = *upValue->location;
        upValue->location = &upValue->closed;
        vm.openUpValues = upValue->next;
    }
}

/**
 * 定义方法
 * @param name
 */
static void defineMethod(ObjectString *name) {
    // 闭包
    Value method = peek(0);
    // 类
    ObjectClass *klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    pop();
}

/**
 * 为方法绑定实例
 * @param klass
 * @param name
 * @return
 */
static bool bindMethod(ObjectClass *klass, ObjectString *name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined method '%s'.", name->chars);
        return false;
    }

    ObjectBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJECT_VAL(bound));
    return true;
}

/**
 * 方法调用
 * @param name
 * @param argCount
 * @return
 */
static bool invokeFromClass(ObjectClass *klass, ObjectString *name, int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

/**
 * 方法调用
 * @param name
 * @param argCount
 * @return
 */
static bool invoke(ObjectString *name, int argCount) {
    Value receiver = peek(argCount);
    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }
    ObjectInstance *instance = AS_INSTANCE(receiver);
    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }
    return invokeFromClass(instance->klass, name, argCount);
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
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
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
        dbgInstruction(&frame->closure->function->chunk, (int) (frame->ip - frame->closure->function->chunk.code));
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                Value result = pop();
                // 从函数返回的时候关闭上值
                closeUpValues(frame->slots);
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
            case OP_GET_UP_VALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upValues[slot]->location);
                break;
            }
            case OP_SET_UP_VALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upValues[slot]->location = peek(0);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjectInstance *instance = AS_INSTANCE(peek(0));
                ObjectString *name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); // Instance.
                    push(value);
                    break;
                }
                // 方法
                if (bindMethod(instance->klass, name)) {
                    break;
                }
                runtimeError("Undefined property '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjectInstance *instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
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
            case OP_INVOKE: {
                ObjectString *method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
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
            case OP_CLOSURE: {
                ObjectFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjectClosure *closure = newClosure(function);
                push(OBJECT_VAL(closure));
                for (int i = 0; i < closure->upValueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upValues[i] = captureUpValue(frame->slots + index);
                    } else {
                        closure->upValues[i] = frame->closure->upValues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UP_VALUE:
                // 此时这个局部变量已经被闭包捕捉了
                closeUpValues(vm.stackTop - 1);
                pop();
                break;
            case OP_CLASS:
                push(OBJECT_VAL(newClass(READ_STRING())));
                break;
            case OP_METHOD:
                defineMethod(READ_STRING());
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
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
    free(vm.grayStack);
}

void initVM() {
    resetStack();
    vm.objects = NULL;

    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.strings);
    initTable(&vm.globals);

    vm.initString = copyString("init", 4);

    defineNative("clock", clockNative);
}

void freeVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    vm.initString = NULL;
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
    ObjectClosure *closure = newClosure(function);
    pop();
    push(OBJECT_VAL(closure));
    call(closure, 0);

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
    // 避免字符串被GC回收
    push(OBJECT_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();
}

ObjectString *findSting(const char *chars, int length, u_int32_t hash) {
    return tableFindKey(&vm.strings, chars, length, hash);
}

void markRoots() {
    // 栈中局部变量
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }
    // 调用栈
    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Object *) vm.frames[i].closure);
    }
    // 上值
    for (ObjectUpValue *upValue = vm.openUpValues;
         upValue != NULL;
         upValue = upValue->next) {
        markObject((Object *) upValue);
    }
    // 全局变量
    markTable(&vm.globals);
    // 编译器：函数
    markCompilerRoots();

    // 驻留
    markObject((Object *) vm.initString);
}

void addGray(Object *object) {
    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Object **) realloc(vm.grayStack, sizeof(Object *) * vm.grayCapacity);
        if (vm.grayStack == NULL) {
            dbg("Error when realloc new grayStack");
            exit(1);
        }
    }

    vm.grayStack[vm.grayCount++] = object;
}

void traceReferences() {
    while (vm.grayCount > 0) {
        Object *object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

void sweepStrings() {
    tableRemoveWhite(&vm.strings);
}

void sweep() {
    Object *previous = NULL;
    Object *object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Object *unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

bool addBytesAllocated(size_t diff) {
    vm.bytesAllocated += diff;
    return vm.bytesAllocated > vm.nextGC;
}

size_t getBytesAllocated() {
    return vm.bytesAllocated;
}

size_t getNextGC() {
    return vm.nextGC;
}

void freshNextGC() {
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
}