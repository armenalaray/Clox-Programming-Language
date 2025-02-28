/* date = February 21st 2025 9:20 pm */

#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Alejandro;
typedef struct ObjString Alejandro2;

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
}ValueType;



typedef struct 
{
    ValueType type;
    
    union{
        bool boolean;
        double number;
        Obj * obj;
    } as;
    
}Value;

/*
Obj is the base class!
*/

#define BOOL_VAL(value) {VAL_BOOL, {.boolean = value}}
#define NIL_VAL {VAL_NIL, {.number = 0}}
#define NUMBER_VAL(value) {VAL_NUMBER, {.number = value}}
#define OBJ_VAL(object) {VAL_OBJ, {.obj = (Obj *)object}}

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)


typedef struct {
    int capacity;
    int count;
    Value* values;
}ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

void printValue(Value value);

#endif //VALUE_H
