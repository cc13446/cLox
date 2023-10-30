//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"
#include "object.h"

#define GC_HEAP_GROW_FACTOR 2

#define ALLOCATE(type, count) \
        (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) \
        ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
        (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
        reallocate(pointer, sizeof(type) * (oldCount), 0)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

/**
 * 释放对象
 */
void freeObject(Object *object);

/**
 * 垃圾回收
 */
void collectGarbage();

/**
 * 标记值
 * @param value
 */
void markValue(Value value);

/**
 * 标记对象
 * @param object
 */
void markObject(Object *object);

/**
 * 黑化对象
 */
void blackenObject(Object *object);

#endif //CLOX_MEMORY_H
