/* date = February 22nd 2025 0:16 pm */

#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "table.h"
#include "chunk.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;


typedef struct 
{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
}CallFrame;

//los roots son cosas que a fuerzas no vas a borrar

typedef struct
{
    //grises
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    
    Value stack[STACK_MAX];
    Value * stackTop;
    
    Table globals;
    
    ObjUpvalue* openUpvalues;
    
    //blanca
    Table strings;
    
    
    //todos
    Obj* objects;
    
    //grabage collector
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    
}VM;

extern VM vm;

void initVM();
void freeVM();

InterpretResult interpret(const char* source);

void push(Value value);
Value pop();

#endif //VM_H
