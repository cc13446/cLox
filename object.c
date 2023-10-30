//
// Created by chen chen on 2023/10/26.
//

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJECT(type, objectType) \
        (type*)allocateObject(sizeof(type), objectType)

/**
 * 为对象分配空间
 * @param size
 * @param type
 * @return
 */
static Object *allocateObject(size_t size, ObjectType type) {
    Object *object = (Object *) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    addObject(object);
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void *) object, size, type);
#endif
    return object;
}

/**
 * 为字符串对象分配空间
 * @param chars
 * @param length
 * @return
 */
static ObjectString *allocateString(char *chars, int length, uint32_t hash) {
    ObjectString *string = ALLOCATE_OBJECT(ObjectString, OBJECT_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    holdString(string);
    return string;
}

/**
 * 对字符串进行hash
 * @param key
 * @param length
 * @return
 */
static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

/**
 * 打印函数
 * @param function
 */
static void printFunction(ObjectFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

ObjectString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjectString *interned = findSting(chars, length, hash);
    if (interned != NULL) {
        return interned;
    }
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjectString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjectString *interned = findSting(chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
}

void printObject(Value value) {
    switch (OBJECT_TYPE(value)) {
        case OBJECT_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJECT_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJECT_NATIVE:
            printf("<native fn>");
            break;
        case OBJECT_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJECT_UP_VALUE:
            printf("upValue");
            break;
    }
}

ObjectFunction *newFunction() {
    ObjectFunction *function = ALLOCATE_OBJECT(ObjectFunction, OBJECT_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    function->upValueCount = 0;
    return function;
}

ObjectNative *newNative(NativeFn function) {
    ObjectNative *native = ALLOCATE_OBJECT(ObjectNative, OBJECT_NATIVE);
    native->function = function;
    return native;
}

ObjectClosure *newClosure(ObjectFunction *function) {
    ObjectUpValue **upValues = ALLOCATE(ObjectUpValue*, function->upValueCount);
    for (int i = 0; i < function->upValueCount; i++) {
        upValues[i] = NULL;
    }
    ObjectClosure *closure = ALLOCATE_OBJECT(ObjectClosure, OBJECT_CLOSURE);
    closure->function = function;
    closure->upValues = upValues;
    closure->upValueCount = function->upValueCount;
    return closure;
}

ObjectUpValue *newUpValue(Value *slot) {
    ObjectUpValue *upValue = ALLOCATE_OBJECT(ObjectUpValue, OBJECT_UP_VALUE);
    upValue->closed = NIL_VAL;
    upValue->location = slot;
    upValue->next = NULL;
    return upValue;
}