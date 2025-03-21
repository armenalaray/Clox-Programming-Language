
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
    obj->isMarked = false;
    obj->next = vm.objects;
    vm.objects = obj;
    
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)obj, size, type);
#endif
    
    return obj;
}

ObjInstance* newInstance(ObjClass* klass)
{
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjClass* newClass(ObjString* name)
{
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    
    klass->name = name;
    initTable(&klass->methods);
    return klass;
}

ObjClosure* newClosure(ObjFunction* function)
{
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    
    for(int i = 0; i < function->upvalueCount; ++i)
    {
        upvalues[i] = NULL;
    }
    
    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    
    return closure;
}

static ObjString * allocateString(char * chars, int length, uint32_t hash)
{
    //si funciona porque estoy allocando el sizeof(ObjString)
    ObjString * string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    
    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();
    
    return string;
}

ObjFunction* newFunction()
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->upvalueCount = 0;
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    
    return function;
}

ObjNative* newNative(NativeFn function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE); 
    
    native->function = function;
    return native;
}

ObjUpvalue* newUpvalue(Value* slot)
{
    //es un puntero
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue,OBJ_UPVALUE); 
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
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
    
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    
    if(interned != NULL) 
    {
        //this is because it is null terminated
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    
    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length)
{
    uint32_t hash = hashString(chars, length);
    
    //es una optimización porq ya no vuelves a crear ese string!
    //si encontramos ese puntero regresa ese puntero!
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    
    if(interned != NULL) return interned;
    
    char * heap = ALLOCATE(char, length + 1);
    memcpy(heap, chars, length);
    heap[length] = '\0';
    
    return allocateString(heap, length, hash);
}

static void printFunction(ObjFunction* function)
{
    if(function->name == NULL) 
    {
        printf("<script>");
        return;
    }
    
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value)
{
    switch(OBJ_TYPE(value))
    {
        case OBJ_INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
        break;

        case OBJ_CLASS:
        printf("%s", AS_CLASS(value)->name->chars);
        break;
        
        case OBJ_CLOSURE:
        printFunction(AS_CLOSURE(value)->function);
        break;
        
        case OBJ_UPVALUE:
        printf("upvalue");
        break;
        
        
        case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
        
        case OBJ_FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
        
        case OBJ_NATIVE:
        printf("<native fn>");
        break;
    }
}