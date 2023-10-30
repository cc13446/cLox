//
// Created by chen chen on 2023/10/15.
//

#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "debug.h"
#include "vm.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    bool gc = addBytesAllocated(newSize - oldSize);
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
    }
    if (gc) {
        collectGarbage();
    }
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    void *result = realloc(pointer, newSize);
    if (result == NULL) {
        dbg("Error when realloc new array");
        exit(1);
    }
    return result;
}

/**
 * 释放对象
 */
void freeObject(Object *object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void *) object, object->type);
#endif
    switch (object->type) {
        case OBJECT_BOUND_METHOD:
            FREE(ObjectBoundMethod, object);
            break;
        case OBJECT_INSTANCE: {
            ObjectInstance *instance = (ObjectInstance *) object;
            freeTable(&instance->fields);
            FREE(ObjectInstance, object);
            break;
        }
        case OBJECT_STRING: {
            ObjectString *string = (ObjectString *) object;
            dbg("Free Memory of Object String %s", string->chars);
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjectString, object);
            break;
        }
        case OBJECT_FUNCTION: {
            ObjectFunction *function = (ObjectFunction *) object;
            freeChunk(&function->chunk);
            FREE(ObjectFunction, object);
            break;
        }
        case OBJECT_NATIVE:
            FREE(ObjectNative, object);
            break;
        case OBJECT_CLOSURE: {
            ObjectClosure *closure = (ObjectClosure *) object;
            FREE_ARRAY(ObjectUpValue *, closure->upValues, closure->upValueCount);
            FREE(ObjectClosure, object);
            break;
        }
        case OBJECT_UP_VALUE:
            FREE(ObjectUpValue, object);
            break;
        case OBJECT_CLASS: {
            ObjectClass *klass = (ObjectClass *) object;
            freeTable(&klass->methods);
            FREE(ObjectClass, object);
            break;
        }
    }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

    size_t before = getBytesAllocated();

    markRoots();
    traceReferences();
    sweepStrings();
    sweep();
    freshNextGC();

    size_t after = getBytesAllocated();

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - after, before, after, getNextGC());
#endif
}

void markValue(Value value) {
    if (IS_OBJECT(value)) {
        markObject(AS_OBJECT(value));
    }
}

void markObject(Object *object) {
    if (object == NULL) {
        return;
    }
    if (object->isMarked) {
        return;
    }
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *) object);
    printValue(OBJECT_VAL(object));
    printf("\n");
#endif
    object->isMarked = true;
    addGray(object);
}

static void markArray(ValueArray *array) {
    for (int i = 0; i < array->size; i++) {
        markValue(array->values[i]);
    }
}

void blackenObject(Object *object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *) object);
    printValue(OBJECT_VAL(object));
    printf("\n");
#endif
    switch (object->type) {
        case OBJECT_BOUND_METHOD: {
            ObjectBoundMethod *bound = (ObjectBoundMethod *) object;
            markValue(bound->receiver);
            markObject((Object *) bound->method);
            break;
        }
        case OBJECT_INSTANCE: {
            ObjectInstance *instance = (ObjectInstance *) object;
            markObject((Object *) instance->klass);
            markTable(&instance->fields);
            break;
        }
        case OBJECT_CLASS: {
            ObjectClass *klass = (ObjectClass *) object;
            markObject((Object *) klass->name);
            markTable(&klass->methods);
            break;
        }
        case OBJECT_CLOSURE: {
            ObjectClosure *closure = (ObjectClosure *) object;
            markObject((Object *) closure->function);
            for (int i = 0; i < closure->upValueCount; i++) {
                markObject((Object *) closure->upValues[i]);
            }
            break;
        }
        case OBJECT_FUNCTION: {
            ObjectFunction *function = (ObjectFunction *) object;
            markObject((Object *) function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJECT_UP_VALUE:
            markValue(((ObjectUpValue *) object)->closed);
            break;
        case OBJECT_NATIVE:
        case OBJECT_STRING:
            break;
    }
}
