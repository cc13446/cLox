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
    addObject(object);
    return object;
}

/**
 * 为字符串对象分配空间
 * @param chars
 * @param length
 * @return
 */
static ObjectString *allocateString(char *chars, int length) {
    ObjectString *string = ALLOCATE_OBJECT(ObjectString, OBJECT_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjectString *copyString(const char *chars, int length) {
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

ObjectString* takeString(char* chars, int length) {
    return allocateString(chars, length);
}

void printObject(Value value) {
    switch (OBJECT_TYPE(value)) {
        case OBJECT_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}