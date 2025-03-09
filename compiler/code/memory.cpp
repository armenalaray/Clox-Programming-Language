
#include <stdlib.h>
#include "memory.h"
#include "vm.h"
#include "object.h"

void * reallocate(void * pointer, size_t oldSize, size_t newSize)
{
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
    switch(obj->type)
    {
        case OBJ_FUNCTION:
        {
            ObjFunction* funcion = (ObjFunction*)obj;
            freeChunk(&funcion->chunk);
            FREE(ObjFunction, obj);
            
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