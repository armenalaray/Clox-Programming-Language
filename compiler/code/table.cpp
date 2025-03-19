
#include "string.h"
#include "memory.h"
#include "table.h"

void initTable(Table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}


void freeTable(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key)
{
    uint32_t index = key->hash % capacity;
    
    Entry* tombstone = NULL;
    //siempre va a haber un nulo porque el capacity se aumenta antes de que se llene
    for(;;)
    {
        Entry* entry = &entries[index];
        
        if(entry->key == NULL)
        {
            if(IS_NIL(entry->value))
            {
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                if(tombstone == NULL) tombstone = entry;
            }
        }
        
        else if(entry->key == key)
        {
            return entry;
        }
        
        //hay una colision!
        index = (index+1) % capacity;
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash)
{
    if(table->count == 0) return NULL;
    
    //la hash sirve para cualquier tabla!
    uint32_t index = hash % table->capacity;
    
    for(;;)
    {
        Entry* entry = &table->entries[index];
        
        //solo te importa la key!
        if(entry->key == NULL)
        {
            if(IS_NIL(entry->value)) return NULL;
        }
        //es deterministica!
        else if(entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key;
        }
        
        index = (index+1) % table->capacity;
    }
    
}

static void adjustCapacity(Table * table, int capacity)
{
    Entry* entries = ALLOCATE(Entry, capacity);
    
    for(int i = 0; i < capacity; ++i)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }
    
    //aqui se borran las tombstones!
    table->count = 0;
    //esta es la viejita
    for(int i = 0; i < table->capacity; ++i)
    {
        Entry* entry = &table->entries[i];
        
        if(entry->key == NULL) continue;
        
        //te dice donde insertarla
        //reinsertas en la nueva
        Entry* dest = findEntry(entries, capacity, entry->key);
        
        dest->key = entry->key;
        dest->value = entry->value;
        ++table->count;
    }
    
    //falta borrar la viejita!
    FREE_ARRAY(Entry, table->entries, table->capacity);
    
    //aqui se asigna
    table->entries = entries;
    table->capacity = capacity;
}

#define TABLE_MAX_LOAD 0.75

//este es mas loosy porque no te importa en donde guradarla aca si!
bool tableSet(Table * table, ObjString* key, Value value)
{
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    
    //esto es aparte
    Entry * entry = findEntry(table->entries, table->capacity, key);
    
    //high level
    bool isNew = entry->key == NULL;
    //low level
    if(isNew && IS_NIL(entry->value)) ++table->count;
    
    //se incerta en el tombstone
    entry->key = key;
    entry->value = value;
    
    return isNew;
}

bool tableGet(Table* table, ObjString* key, Value* value)
{
    if(table->count == 0) return false;
    
    //if(key->chars
    
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if(entry->key == NULL) return false;
    
    *value = entry->value;
    return true;
}

//the count is the number of entries + tombstones!
bool tableDelete(Table* table, ObjString* key)
{
    if(table->count == 0) return false;
    
    Entry* entry = findEntry(table->entries, table->capacity, key);
    //this means we cannot delete it because it doesnt exist!
    //es completamente falso!
    if(entry->key == NULL) return false;
    
    //here it means we got it!
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    
    return false;
}

void tableAddAll(Table* from, Table* to)
{
    for(int i = 0; i < from->capacity; ++i)
    {
        Entry* entry = &from->entries[i];
        if(entry->key != NULL) tableSet(to, entry->key, entry->value);
    }
}

//esta pueden ser cualquier tipo de objeto como funcion etc.
void markTable(Table* table)
{
    for(int i = 0; i < table->capacity; ++i)
    {
        //esque las dos pueden estar heap allocated
        Entry* entry = &table->entries[i];
        
        markObject((Obj*)entry->key);
        
        markValue(entry->value);
    }
}


void tableRemoveWhite(Table* table)
{
    for(int i = 0; i < table->capacity; ++i)
    {
        Entry* entry = &table->entries[i];
        if(entry->key != NULL && !entry->key->obj.isMarked)
            tableDelete(table, entry->key);
    }
}












