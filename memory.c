//
// Created by chen chen on 2023/10/15.
//

#include <stdlib.h>

#include "memory.h"
#include "dbg.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    void* result = realloc(pointer, newSize);
    if (result == NULL) {
        dbg("Error when realloc new array")
        exit(1);
    }
    return result;
}


