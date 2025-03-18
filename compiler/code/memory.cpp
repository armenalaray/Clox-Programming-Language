
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
    if(newSize > oldSize)
    {
#ifdef DEBUG_STRESS_GC
        //entonces se llamaría a si misma hehe!
        //garbageCollect llama a reallocate() para hacer free!
        collectGarbage();
        //for allocation!
        //during call boundaries!
        //backward jumps!
#endif
    }
    
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }
    
    /*
Todo lo hace realloc!
*/
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
    
#ifdef DEBUG_LOG_GC
    //tecnicamente estas imprmiendo el entry
    printf("%p mark ", (void*)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif
    obj->isMarked = true;
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
}

void collectGarbage()
{
    
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif
    
    markRoots();
    
#ifdef DEBUG_LOG_GC
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
}