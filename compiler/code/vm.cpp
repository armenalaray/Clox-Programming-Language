
#include <time.h>
#include "string.h"
#include <stdarg.h>
#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"

VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
};

void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

static Value peek(int distance)
{
    return vm.stackTop[-1-distance];
}

bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));
    
    int length = a->length + b->length;
    //it is because here we have allocate we need to free it!
    char * chars = ALLOCATE(char, length + 1);
    
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    
    chars[length] = '\0';
    
    ObjString* result = takeString(chars, length);
    
    pop();
    pop();
    
    push(OBJ_VAL(result));
}


//No te va a jalar nada por que no tiene args!
static void runtimeError(const char * format, ...)
{
    va_list args;
    //esta optiene los argumentos 
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    
    //stack trace
    for(int i = vm.frameCount - 1; i >= 0; --i)
    {
        CallFrame* frame = &vm.frames[i];
        
        ObjFunction* function = frame->closure->function;
        
        size_t instruction = frame->ip - 1 - function->chunk.code;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        
        if(function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    
    resetStack();
}

static void defineNative(const char* name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

bool call(ObjClosure* closure, int argCount)
{
    //aqui ya esta creada la funcion!
    
    if(argCount != closure->function->arity)
    {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }
    
    if(vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Stack Overflow.");
        return false;
    }
    
    //aqui esta pero no importa!
    CallFrame* frame = &vm.frames[vm.frameCount++];
    
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount)
{
    if(IS_OBJ(callee))
    {
        switch(OBJ_TYPE(callee))
        {
            case OBJ_CLASS:
            {
                ObjClass* klass = AS_CLASS(callee);
                vm.stackTop[-1-argCount] = OBJ_VAL(newInstance(klass));                
                return true;
            }

            case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), argCount);
            
            case OBJ_NATIVE:
            {
                //esta harcodeada!
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            
            default:
            break;
        }
    }
    
    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue* captureUpvalue(Value* local)
{
    //the list is sorted
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    
    /*

prev va a ser mi prev

createdUpvalue

siempre up value va a ser el next

estan sorteadas de mayor a menor!

ya no se repiten
*/
    while(upvalue != NULL && upvalue->location > local)
    {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    if(upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }
    
    ObjUpvalue* createdUpvalue = newUpvalue(local);
    
    /*
esta insertando en medio
sorteadas
*/
    
    createdUpvalue->next = upvalue;
    
    if(prevUpvalue == NULL)
    {
        vm.openUpvalues = createdUpvalue;
    }
    else
    {
        prevUpvalue->next = createdUpvalue;
    }
    
    return createdUpvalue;
}

static void closeUpvalues(Value* last)
{
    while(vm.openUpvalues != NULL && vm.openUpvalues->location >= last)
    {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjString* name)
{
    Value method = peek(0);
    ObjClass* klass =  AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    pop();
}

static InterpretResult run()
{
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
(frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
    
#define READ_STRING() AS_STRING(READ_CONSTANT())
    
#define BINARY_OP(valueType, op) \
do { \
if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))){\
runtimeError("Operands must be numbers.");\
return INTERPRET_RUNTIME_ERROR; \
}\
double b = AS_NUMBER(pop()); \
double a = AS_NUMBER(pop()); \
push(valueType(a op b)); \
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
        
        disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
#endif
        
        uint8_t instruction;
        switch(instruction = READ_BYTE())
        {
            //it executes
            case OP_CONSTANT:
            {
                //Solo imprimes la constante
                //aqui no esta en el stack
                Value constant = READ_CONSTANT();
                //printValue(constant);
                push(constant);
                //printf("\n");
                break;
            }
            
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            
            
            case OP_GET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            
            case OP_SET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            
            case OP_GET_GLOBAL:
            {
                ObjString* name = READ_STRING();
                Value value;
                if(!tableGet(&vm.globals, name, &value))
                {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                push(value);
                break;
            }
            
            case OP_SET_GLOBAL:
            {
                ObjString* name = READ_STRING();
                //NOTE: since it is new we need to delete it because we expect to be defined before
                //tableset funciona hacer set y insertar nueva
                if(tableSet(&vm.globals, name, peek(0)))
                {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                //aqui no se hace el pop!
                break;
            }
            
            case OP_DEFINE_GLOBAL:
            {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            
            case OP_EQUAL:
            {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a,b)));
                break;
            }
            case OP_GREATER:
            {
                BINARY_OP(BOOL_VAL,>);
                break;
            }
            case OP_LESS:
            {
                BINARY_OP(BOOL_VAL,<);
                break;
            }
            
            case OP_ADD:
            {
                if(IS_STRING(peek(0)) && IS_STRING(peek(1)))
                {
                    concatenate();
                }
                else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
                {
                    double b = AS_NUMBER(pop()); 
                    double a = AS_NUMBER(pop()); 
                    push(NUMBER_VAL(a + b)); 
                }
                else
                {
                    runtimeError("Operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR; 
                }
                
                break;
            }
            
            case OP_SUBTRACT:
            {
                BINARY_OP(NUMBER_VAL,-);
                break;
            }
            case OP_MULTIPLY:
            {
                BINARY_OP(NUMBER_VAL,*);
                break;
            }
            case OP_DIVIDE:
            {
                BINARY_OP(NUMBER_VAL,/);
                break;
            }
            case OP_NOT:
            {
                push(BOOL_VAL(isFalsey(pop())));
                break;
            }
            case OP_NEGATE:
            {
                if(!IS_NUMBER(peek(0)))
                {
                    //osea no lo puedes ver hasta que se corra
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_PRINT:
            {
                printValue(pop());
                printf("\n");
                break;
            }
            
            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();
                
                if(isFalsey(peek(0))) frame->ip += offset;
                
                
                break;
            }
            
            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                
                break;
            }
            
            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                
                break;
            };
            
            case OP_CALL:
            {
                int argCount = READ_BYTE();
                //este es el objeto al q vas a llamar
                //ahi esta un value con la
                if(!callValue(peek(argCount), argCount))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_METHOD:
            {
                defineMethod(READ_STRING());
                break;
            }

            case OP_SET_PROPERTY:
            {
                if(!IS_INSTANCE(peek(1)))
                {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0)); 
                Value value = pop();
                pop();
                push(value);               
                break;
            }

            case OP_GET_PROPERTY:
            { 
                
                if(!IS_INSTANCE(peek(0)))
                {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                ObjInstance* instance = AS_INSTANCE(peek(0)); 
                ObjString* name = READ_STRING();

                Value value;
                if(tableGet(&instance->fields, name, &value))
                {
                    pop();
                    push(value);
                    break;
                }

                runtimeError("Undefined property '%s'.", name->chars);
                break;
            }

            case OP_CLASS:
            {
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
            }

            case OP_CLOSURE:
            {
                //es lo mismo q una constante 
                //esta es la que esta adentra
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));
                
                for(int i = 0; i < closure->upvalueCount; ++i)
                {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    
                    if(isLocal)
                    {
                        //son como cajitas
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                
                
                break;
            }
            
            case OP_GET_UPVALUE:
            {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            
            case OP_SET_UPVALUE:
            {
                uint8_t slot = READ_BYTE();
                //no se hace pop!
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            
            case OP_CLOSE_UPVALUE:
            {
                //aqui esta en el heap
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            }
            
            case OP_RETURN:
            {
                Value result = pop();
                
                
                --vm.frameCount;
                
                //esto se transforma en un resultado!
                if(vm.frameCount == 0)
                {
                    //objfunction*
                    pop();
                    return INTERPRET_OK;
                }
                
                //lo tiene que cerrar!
                vm.stackTop = frame->slots;
                
                closeUpvalues(frame->slots);
                
                push(result);
                
                
                frame = &vm.frames[vm.frameCount - 1];
                
                break;
            }
            
        }
        
    }
    
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
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
    //aqui se compile pero no hay nada en el stack!
    //tienes un puntero y eso se aloca automaticamente
    ObjFunction* function = compile(source);
    
    if(function == NULL) return INTERPRET_COMPILE_ERROR;
    
    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    
    push(OBJ_VAL(closure));
    
    call(closure, 0);
    
    return run();
}

static Value clockNative(int argCount, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}


void initVM()
{
    resetStack();
    vm.objects = NULL;
    
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    
    vm.bytesAllocated = 0;
    vm.nextGC = 1024*1024;
    //vm.nextGC = 64;
    
    initTable(&vm.strings);
    initTable(&vm.globals);
    
    defineNative("clock", clockNative);
    
}
