
#include "stdio.h"

#include "string.h"
#include "memory.h"
#include "vm.h"
#include "object.h"

#define ALLOCATE_OBJ(type, objectType)\
(type*)allocateObject(sizeof(type), objectType)

//NOTE: Aqui se crea el objeto completo no solo el obj
static Obj * allocateObject(size_t size, ObjType type)
{
    Obj * obj = (Obj*)reallocate(NULL,0,size);
    //This is the only one you can access directly since its the first one!
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

static ObjString * allocateString(char * chars, int length, uint32_t hash)
{
    //si funciona porque estoy allocando el sizeof(ObjString)
    ObjString * string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    return string;
}

//FNV-1a hash function
static uint32_t hashString(const char * key, int length)
{
    //FNV offset basis
    uint32_t hash = 2166136261u;
    for(int i=0; i < length; ++i)
    {
        //XOR hash
        hash ^= (uint8_t)key[i];
        hash *= 16777619; //FNV prime
    }
    return hash;
}

ObjString* takeString(char* chars, int length)
{
    uint32_t hash = hashString(chars, length);
    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length)
{
    uint32_t hash = hashString(chars, length);
    char * heap = ALLOCATE(char, length + 1);
    memcpy(heap, chars, length);
    heap[length] = '\0';
    
    return allocateString(heap, length, hash);
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