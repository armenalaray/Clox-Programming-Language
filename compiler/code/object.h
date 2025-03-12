/* date = February 28th 2025 0:49 pm */

#ifndef clox_object_h
#define clox_object_h

#include "value.h"
#include "chunk.h"

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    
}ObjType;

struct Obj
{
    ObjType type;
    struct Obj* next;
};

struct ObjString
{
    Obj obj;
    int length;
    char * chars;
    uint32_t hash;
};

typedef struct
{
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
}ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct
{
    Obj obj;
    NativeFn function;
}ObjNative;


#define OBJ_TYPE(value) (AS_OBJ(value)->type)

//Va a haber una main function!
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

#endif //OBJECT_H
