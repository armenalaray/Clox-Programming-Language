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


void initTable(Table* table);
void freeTable(Table* table);

#endif //TABLE_H
