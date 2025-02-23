/* date = February 22nd 2025 0:16 pm */

#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "compiler.h"

#define STACK_MAX 256

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct
{
    uint8_t * ip;
    Chunk * chunk;
    Value stack[STACK_MAX];
    Value * stackTop;
}VM;

void initVM();
void freeVM();

InterpretResult interpret(Chunk* chunk);

InterpretResult interpret(const char* source);

void push(Value value);
Value pop();

#endif //VM_H
