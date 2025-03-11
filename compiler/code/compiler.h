/* date = February 22nd 2025 9:37 pm */

#ifndef clox_compiler_h
#define clox_compiler_h

#include "scanner.h"
#include "chunk.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
}Precedence;

typedef void (*ParseFn)(bool canAssign);

struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

typedef struct
{
    Token name;
    int depth;
}Local;

typedef enum
{
    TYPE_FUNCTION,
    TYPE_SCRIPT
}FunctionType;

typedef struct Compiler
{
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;
    
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
}Compiler;

typedef struct 
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
}Parser;

void string(bool canAssign);
void and_(bool canAssign);
void or_(bool canAssign);
void literal(bool canAssign);
void grouping(bool canAssign);
void unary(bool canAssign);
void binary(bool canAssign);
void number(bool canAssign);
void variable(bool canAssign);
void declaration();
void statement();

ObjFunction* compile(const char* source);

#endif //COMPILER_H
