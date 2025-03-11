/* date = February 22nd 2025 0:16 pm */

#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "table.h"
#include "chunk.h"
#include "compiler.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;


typedef struct 
{
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
}CallFrame;


typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    
    Value stack[STACK_MAX];
    Value * stackTop;
    
    Table globals;
    Table strings;
    
    //linked list
    Obj* objects;
}VM;

extern VM vm;

void initVM();
void freeVM();

InterpretResult interpret(const char* source);

void push(Value value);
Value pop();

#endif //VM_H
