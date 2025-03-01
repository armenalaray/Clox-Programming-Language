
#include "stdio.h"

#include "string.h"
#include "memory.h"
#include "object.h"

#define ALLOCATE_OBJ(type, objectType)\
(type*)allocateObject(sizeof(type), objectType)

//NOTE: Aqui se crea el objeto completo no solo el obj
static Obj * allocateObject(size_t size, ObjType type)
{
    Obj * obj = (Obj*)reallocate(NULL,0,size);
    //This is the only one you can access directly since its the first one!
    obj->type = type;
    return obj;
}

static ObjString * allocateString(char * chars, int length)
{
    //si funciona porque estoy allocando el sizeof(ObjString)
    ObjString * string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* takeString(char* chars, int length)
{
    return allocateString(chars, length);
}

ObjString* copyString(const char* chars, int length)
{
    char * heap = ALLOCATE(char, length + 1);
    memcpy(heap, chars, length);
    heap[length] = '\0';
    
    return allocateString(heap, length);
}

void printObject(Value value)
{
    switch(OBJ_TYPE(value))
    {
        case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}