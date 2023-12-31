//
// Created by chen chen on 2023/10/25.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"

/**
 * 调用栈
 */
typedef struct {
    ObjectClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

/**
 * 虚拟机
 */
typedef struct {
    CallFrame frames[FRAMES_MAX];   // 调用栈
    int frameCount;                 // 调用栈数量
    Value stack[STACK_MAX];         // 虚拟机栈
    Value *stackTop;                // 虚拟机栈顶
    Object *objects;                // 所有对象的链表
    Table strings;                  // 字符串常量池
    Table globals;                  // 全局变量
    ObjectUpValue *openUpValues;    // 被关闭的上值

    int grayCount;
    int grayCapacity;
    Object **grayStack;

    size_t bytesAllocated;
    size_t nextGC;

    ObjectString* initString;
} VM;

/**
 * 字节码执行结果
 */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

/**
 * 初始化虚拟机
 */
void initVM();

/**
 * 释放虚拟机
 */
void freeVM();

/**
 * 执行字节码
 * @param chunk
 * @return
 */
InterpretResult interpret(const char *source);

/**
 * 入栈
 * @param value
 */
void push(Value value);

/**
 * 栈顶元素出栈
 * @return
 */
Value pop();

/**
 * 栈顶元素
 * @param distance
 * @return
 */
Value peek(int distance);

/**
 * 添加一个堆对象，用于内存释放
 * @param object
 */
void addObject(Object *object);

/**
 * 将字符串存入常量池
 * @param string
 */
void holdString(ObjectString *string);

/**
 * 从常量池寻找字符串
 * @param string
 */
ObjectString *findSting(const char *chars, int length, u_int32_t hash);

/**
 * 扫描根节点
 */
void markRoots();

/**
 * 添加灰色节点
 */
void addGray(Object *object);

/**
 * 跟踪引用
 */
void traceReferences();

/**
 * 清除字符串常量池
 */
void sweepStrings();

/**
 * 清除对象
 */
void sweep();

/**
 * 统计字节码
 * @param diff
 */
bool addBytesAllocated(size_t diff);

/**
 * 更新GC阈值
 */
void freshNextGC();

/**
 * 获取分配的字节码
 * @return
 */
size_t getBytesAllocated();

/**
 * 获取下一次触发GC的字节数
 * @return
 */
size_t getNextGC();

#endif //CLOX_VM_H
