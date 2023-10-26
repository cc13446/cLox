//
// Created by chen chen on 2023/10/26.
//

#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "common.h"
#include "value.h"

typedef struct {
    ObjectString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

/**
 * 初始化哈希表
 * @param table
 */
void initTable(Table *table);

/**
 * 释放哈希表
 * @param table
 */
void freeTable(Table *table);

/**
 * 将对象放入哈希表
 * @param table
 * @param key
 * @param value
 * @return
 */
bool tableSet(Table *table, ObjectString *key, Value value);

/**
 * 从哈希表中获取值
 * @param table
 * @param key
 * @param value
 * @return
 */
bool tableGet(Table *table, ObjectString *key, Value *value);

/**
 * 从哈希表中删除条目
 * @param table
 * @param key
 * @return
 */
bool tableDelete(Table *table, ObjectString *key);

/**
 * table 复制
 * @param from
 * @param to
 */
void tableAddAll(Table *from, Table *to);

/**
 * 从哈希表中寻找Key
 * @param table
 * @param chars
 * @param length
 * @param hash
 * @return
 */
ObjectString* tableFindKey(Table* table, const char* chars, int length, uint32_t hash);

#endif //CLOX_TABLE_H
