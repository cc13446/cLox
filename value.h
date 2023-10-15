//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

/**
 * 字面量
 */
typedef double Value;

typedef struct {
    int capacity;
    int size;
    Value *values;
} ValueArray;

/**
 * 初始化字面量数组
 * @param array
 */
void initValueArray(ValueArray* array);

/**
 * 写入字面量数组
 * @param array
 * @param value
 */
void writeValueArray(ValueArray* array, Value value);

/**
 * 释放字面量数组
 * @param array
 */
void freeValueArray(ValueArray* array);

/**
 * 打印常量
 * @param value
 */
void printValue(Value value);

#endif //CLOX_VALUE_H
