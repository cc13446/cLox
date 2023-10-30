//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include <string.h>

#include "common.h"


/**
 * 对象
 */
typedef struct Object Object;

/**
 * 字符串对象
 */
typedef struct ObjectString ObjectString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)
#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJECT(value)    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJECT(value)    ((Object*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

static inline double valueToNum(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) numToValue(num)
#define OBJECT_VAL(obj) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else
#define IS_BOOL(value)     ((value).type == VAL_BOOL)
#define IS_NIL(value)      ((value).type == VAL_NIL)
#define IS_NUMBER(value)   ((value).type == VAL_NUMBER)
#define IS_OBJECT(value)   ((value).type == VAL_OBJECT)

#define AS_BOOL(value)     ((value).as.boolean)
#define AS_NUMBER(value)   ((value).as.number)
#define AS_OBJECT(value)   ((value).as.object)

#define BOOL_VAL(value)    ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL            ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)  ((Value){VAL_NUMBER, {.number = value}})
#define OBJECT_VAL(value)  ((Value){VAL_OBJECT, {.object = (Object*)value}})

/**
 * 值类型
 */
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJECT
} ValueType;

/**
 * 字面量
 */
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Object *object;
    } as;
} Value;

#endif

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
