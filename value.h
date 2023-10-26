//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

/**
 * 值类型
 */
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

/**
 * 字面量
 */
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

typedef struct {
    int capacity;
    int size;
    Value *values;
} ValueArray;

/**
 * 初始化字面量数组
 * @param array
 */
void initValueArray(ValueArray *array);

/**
 * 写入字面量数组
 * @param array
 * @param value
 */
void writeValueArray(ValueArray *array, Value value);

/**
 * 释放字面量数组
 * @param array
 */
void freeValueArray(ValueArray *array);

/**
 * 打印常量
 * @param value
 */
void printValue(Value value);

/**
 * 比较是否相等
 * @param a
 * @param b
 * @return
 */
bool valuesEqual(Value a, Value b);

#endif //CLOX_VALUE_H
