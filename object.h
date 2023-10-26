//
// Created by chen chen on 2023/10/26.
//

#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJECT_TYPE(value)     (AS_OBJECT(value)->type)

#define IS_STRING(value)       isObjectType(value, OBJECT_STRING)

#define AS_STRING(value)       ((ObjectString*)AS_OBJECT(value))
#define AS_CSTRING(value)      (((ObjectString*)AS_OBJECT(value))->chars)

/**
 * 对象类型
 */
typedef enum {
    OBJECT_STRING,
} ObjectType;

struct Object {
    ObjectType type;
    struct Object* next;  // 用于内存释放
};

struct ObjectString {
    Object object;
    int length;
    char* chars;
};

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

/**
 * 复制出来一个字符串对象
 * @param chars
 * @param length
 * @return
 */
ObjectString* copyString(const char* chars, int length);


/**
 * 获取字符串对象
 * @param chars
 * @param length
 * @return
 */
ObjectString* takeString(char* chars, int length);

/**
 * 打印对象
 * @param value
 */
void printObject(Value value);

#endif //CLOX_OBJECT_H
