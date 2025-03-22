/* date = February 28th 2025 0:49 pm */

#ifndef clox_object_h
#define clox_object_h

#include "table.h"
#include "value.h"
#include "chunk.h"

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
}ObjType;

struct Obj
{
    ObjType type;
    bool isMarked;
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


typedef struct ObjUpvalue
{
    Obj obj;
    Value* location;
    Value closed;
    //esta es la que importa!
    ObjUpvalue* next;
}ObjUpvalue;


//todas las funciones van a ser closures!
typedef struct
{
    Obj obj;
    ObjFunction* function;
    //aqui es donde si puede pasar que ya se hayan 
    ObjUpvalue** upvalues;
    int upvalueCount;
}ObjClosure;

typedef struct
{
    Obj obj;
    ObjString* name;
    Table methods;
}ObjClass;

typedef struct
{
    Obj obj;
    ObjClass* klass;   
    Table fields;
}ObjInstance;

typedef struct
{
    Obj obj;
    Value receiver;
    ObjClosure* method;
}ObjBoundMethod;


#define OBJ_TYPE(value) (AS_OBJ(value)->type)

//Va a haber una main function!
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);

ObjInstance* newInstance(ObjClass* klass);
ObjClass* newClass(ObjString* name);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjClosure* newClosure(ObjFunction* function);
ObjUpvalue* newUpvalue(Value* slot);

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

#endif //OBJECT_H
