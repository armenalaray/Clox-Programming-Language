/* date = March 1st 2025 1:39 pm */

#ifndef TABLE_H
#define TABLE_H

#include "object.h"

typedef struct
{
    ObjString* key;
    Value value;
}Entry;

typedef struct
{
    int count;
    int capacity;
    Entry * entries;
}Table;


void markTable(Table* table);
void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table * table, ObjString* key, Value value);
bool tableGet(Table* table, ObjString* key, Value* value);
void tableAddAll(Table* from, Table* to);
bool tableDelete(Table* table, ObjString* key);

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

#endif //TABLE_H
