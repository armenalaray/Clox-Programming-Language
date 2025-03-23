
#include <stdlib.h>
#include "memory.h"
#include "vm.h"
#include "object.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC

#include <stdio.h>
#include "debug.h"

#endif

void * reallocate(void * pointer, size_t oldSize, size_t newSize)
{
    vm.bytesAllocated += newSize - oldSize;
    
    if(newSize > oldSize)
    {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
        if(vm.bytesAllocated > vm.nextGC)
            collectGarbage();
    }
    
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }
    
    void *result = realloc(pointer, newSize);
    if(result == NULL) exit(1);
    return result;
    
}

void freeObject(Obj* obj)
{
    
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif
    
    switch(obj->type)
    {
        case OBJ_BOUND_METHOD:
        {
            FREE(ObjBoundMethod, obj);
            break;
        }

        case OBJ_INSTANCE:
        {
            //no es dueña de class
            ObjInstance* instance = (ObjInstance*)obj;
            freeTable(&instance->fields);
            FREE(ObjInstance, obj);
            break;
        }

        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)obj;
            freeTable(&klass->methods);
            FREE(ObjClass, obj);
            break;
        }
        
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            //         sizeof element 64 bits
            FREE_ARRAY(ObjClosure*, closure->upvalues, closure->upvalueCount);
            
            FREE(ObjClosure, obj);
            break;
        }
        
        case OBJ_UPVALUE:
        {
            FREE(ObjUpvalue, obj);
            break;
        }
        
        case OBJ_FUNCTION:
        {
            ObjFunction* funcion = (ObjFunction*)obj;
            freeChunk(&funcion->chunk);
            FREE(ObjFunction, obj);
            
            break;
        }
        case OBJ_NATIVE:
        {
            FREE(ObjNative, obj);
            break;
        }
        case OBJ_STRING:
        {
            ObjString* a = (ObjString*)obj;
            FREE_ARRAY(char, a->chars, a->length + 1);
            FREE(ObjString, obj);
            break;
        }
    }
}

void markValue(Value value)
{
    //Only objects are heap allocated!
    if(IS_OBJ(value)) markObject(AS_OBJ(value));
}

void markObject(Obj* obj)
{
    if(obj == NULL) return;
    if(obj->isMarked) return;
    
#ifdef DEBUG_LOG_GC
    //tecnicamente estas imprmiendo el entry
    printf("%p mark ", (void*)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif
    obj->isMarked = true;
    
    if(vm.grayCapacity < vm.grayCount + 1)
    {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        
        if(vm.grayStack == NULL) exit(1);
    }
    
    vm.grayStack[vm.grayCount++] = obj;
}

//esta marca el stack!
static void markRoots()
{
    for(Value* slot = vm.stack; slot <= vm.stackTop; ++slot)
    {
        markValue(*slot);
    }
    
    for(int i = 0; i < vm.frameCount; ++i)
    {
        markObject((Obj*)vm.frames[i].closure);
    }
    
    for(ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue != NULL;
        upvalue = upvalue->next)
    {
        markObject((Obj*)upvalue);
    }
    
    markTable(&vm.globals);
    markCompilerRoots();
    
    markObject((Obj*)vm.initString);
}

static void markArray(ValueArray* array)
{
    for(int i = 0; i < array->count; ++i)
    {
        markValue(array->values[i]);
    }
}

void blackenObject(Obj* obj)
{
#ifdef DEBUG_LOG_GC
    
    printf("%p blacken ", (void*)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
    
#endif
    
    switch(obj->type)
    {
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod* bound = (ObjBoundMethod*)obj;
            markValue(bound->receiver);
            markObject((Obj*)bound->method);
            break;
        }

        case OBJ_INSTANCE:
        {
            ObjInstance* instance = (ObjInstance*)obj;
            markObject((Obj*)instance->klass);
            markTable(&instance->fields);
            break;
        }

        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)obj;
            markObject((Obj*)klass->name);
            markTable(&klass->methods);
            break;
        }

        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            markObject((Obj*)closure->function);
            
            for(int i = 0; i < closure->upvalueCount; ++i)
            {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)obj;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_UPVALUE:
        {
            markValue(((ObjUpvalue*)obj)->closed);
            break;
        }
        case OBJ_STRING:
        case OBJ_NATIVE:
        {
            break;
        }
    }
}

static void traceReferences()
{
    //esta mierda sube!
    while(vm.grayCount > 0)
    {
        //-- --  --  ---
        //-- -- -- -- DEPTH FIRST SEARCH
        Obj* obj = vm.grayStack[--vm.grayCount];
        blackenObject(obj);
    }
}

void sweep()
{
    Obj* prev = NULL;
    Obj* obj = vm.objects;
    while(obj != NULL)
    {
        if(obj->isMarked)
        {
            obj->isMarked = false;
            prev = obj;
            obj = obj->next;
        }
        else
        {
            Obj* unreached = obj;
            
            obj = obj->next;
            
            if(prev == NULL)
                vm.objects = obj;
            else
                prev->next = obj;
            
            freeObject(unreached);
        }
    }
}

void collectGarbage()
{
    
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    
    size_t before = vm.bytesAllocated;
    
#endif
    
    
    
    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();
    
    //solo si se llama al collector crece
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
    
#ifdef DEBUG_LOG_GC
    
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
    
    printf("-- gc end\n");
#endif
}

void freeObjects()
{
    Obj* obj = vm.objects;
    while(obj != NULL)
    {
        Obj * next = obj->next;
        freeObject(obj);
        obj = next;
    }
    
    free(vm.grayStack);
}