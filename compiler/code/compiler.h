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
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
}Parser;

void string(bool canAssign);
void literal(bool canAssign);
void grouping(bool canAssign);
void unary(bool canAssign);
void binary(bool canAssign);
void number(bool canAssign);
void variable(bool canAssign);

bool compile(const char* source, Chunk* chunk);

#endif //COMPILER_H
