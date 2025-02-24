

#include <stdio.h>
#include "vm.h"
#include "debug.h"

VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
};

void initVM()
{
    resetStack();
}

void freeVM()
{
    
}

static InterpretResult run()
{
    
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    
#define BINARY_OP(op) \
do { \
Value b = pop(); \
Value a = pop(); \
push(a op b); \
} while(false)
    
    for(;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        
        for(Value * slot = vm.stack; 
            slot != vm.stackTop;
            ++slot
            )
        {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        
        uint8_t instruction;
        switch(instruction = READ_BYTE())
        {
            //it executes
            case OP_CONSTANT:
            {
                //Solo imprimes la constante
                Value constant = READ_CONSTANT();
                printValue(constant);
                push(constant);
                printf("\n");
                break;
            }
            case OP_ADD:
            {
                BINARY_OP(+);
                break;
            }
            case OP_SUBTRACT:
            {
                BINARY_OP(-);
                break;
            }
            case OP_MULTIPLY:
            {
                BINARY_OP(*);
                break;
            }
            case OP_DIVIDE:
            {
                BINARY_OP(/);
                break;
            }
            case OP_NEGATE:
            {
                push(-pop());
                break;
            }
            case OP_RETURN:
            {
                printValue(pop());
                printf("\n");
                
                return INTERPRET_OK;
            }
        }
        
    }
    
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
    
}

InterpretResult interpret(Chunk* chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

void push(Value value)
{
    *vm.stackTop = value;
    ++vm.stackTop;
}

Value pop()
{
    --vm.stackTop;
    return *vm.stackTop;
}

InterpretResult interpret(const char* source)
{
    Chunk chunk;
    
    initChunk(&chunk);
    
    if(!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    
    InterpretResult result = run();
    
    freeChunk(&chunk);
    
    return result;
}