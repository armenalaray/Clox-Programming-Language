/* date = February 28th 2025 0:49 pm */

#ifndef clox_object_h
#define clox_object_h

#include "value.h"
#include "chunk.h"

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
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
    
    int upvalueCount;
    
    Chunk chunk;
    ObjString* name;
}ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct
{
    Obj obj;
    NativeFn function;
}ObjNative;

//todas las funciones van a ser closures!
typedef struct
{
    Obj obj;
    ObjFunction* function;
}ObjClosure;

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

//Va a haber una main function!
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

#define IS_STRING(value) isObjType(value, OBJ_STRING)


#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjClosure* newClosure(ObjFunction* function);

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

#endif //OBJECT_H
