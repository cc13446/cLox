//
// Created by chen chen on 2023/10/26.
//

#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"
#include "value.h"
#include "chunk.h"

#define OBJECT_TYPE(value)     (AS_OBJECT(value)->type)

#define IS_STRING(value)       isObjectType(value, OBJECT_STRING)
#define IS_FUNCTION(value)     isObjectType(value, OBJECT_FUNCTION)
#define IS_NATIVE(value)       isObjectType(value, OBJECT_NATIVE)
#define IS_CLOSURE(value)      isObjectType(value, OBJECT_CLOSURE)

#define AS_STRING(value)       ((ObjectString*)AS_OBJECT(value))
#define AS_CSTRING(value)      (((ObjectString*)AS_OBJECT(value))->chars)
#define AS_FUNCTION(value)     ((ObjectFunction*)AS_OBJECT(value))
#define AS_NATIVE(value)       (((ObjectNative*)AS_OBJECT(value))->function)
#define AS_CLOSURE(value)      ((ObjectClosure*)AS_OBJECT(value))

/**
 * 对象类型
 */
typedef enum {
    OBJECT_STRING,
    OBJECT_FUNCTION,
    OBJECT_NATIVE,
    OBJECT_CLOSURE,
    OBJECT_UP_VALUE
} ObjectType;

struct Object {
    ObjectType type;
    struct Object *next;  // 用于内存释放
};

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    ObjectType obj;
    NativeFn function;
} ObjectNative;

struct ObjectString {
    Object object;
    int length;
    char *chars;
    uint32_t hash;
};

typedef struct {
    Object object;
    int arity;
    Chunk chunk;
    ObjectString *name;
    int upValueCount;
} ObjectFunction;

typedef struct ObjectUpValue {
    Object object;
    Value *location;
    Value closed;
    struct ObjectUpValue* next;
} ObjectUpValue;

typedef struct {
    Object object;
    ObjectFunction *function;
    ObjectUpValue **upValues;
    int upValueCount;
} ObjectClosure;

/**
 * 为什么不是放在宏里？
 * 宏的展开方式是在主体中形参名称出现的每个地方插入实参表达式。
 * 如果一个宏中使用某个参数超过一次，则该表达式就会被求值多次。
 * 如果这个表达式有副作用，那就不好了。
 * 比如 IS_STRING(POP())
 *
 * @param value
 * @param type
 * @return
 */
static inline bool isObjectType(Value value, ObjectType type) {
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

// ==================== 字符串对象 ====================

/**
 * 复制出来一个字符串对象
 * @param chars
 * @param length
 * @return
 */
ObjectString *copyString(const char *chars, int length);


/**
 * 获取字符串对象
 * @param chars
 * @param length
 * @return
 */
ObjectString *takeString(char *chars, int length);

/**
 * 打印对象
 * @param value
 */
void printObject(Value value);

// ==================== 函数对象 ====================
/**
 * 新建函数对象
 * @return
 */
ObjectFunction *newFunction();

/**
 * 新建本地函数对象
 * @param function
 * @return
 */
ObjectNative *newNative(NativeFn function);

/**
 * 新建闭包对象
 * @param function
 * @return
 */
ObjectClosure *newClosure(ObjectFunction *function);

/**
 * 新建上值对象
 * @param function
 * @return
 */
ObjectUpValue *newUpValue(Value *slot);

#endif //CLOX_OBJECT_H
