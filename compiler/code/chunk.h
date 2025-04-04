/* date = February 21st 2025 4:48 pm */

#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT,
    OP_CLOSURE,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_CLASS,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_INVOKE,
    OP_METHOD,
    OP_INHERIT,
    OP_GET_SUPER,
    OP_SUPER_INVOKE,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_LOOP,
    OP_CALL,
    OP_JUMP_IF_FALSE,
    OP_JUMP, //este es incondicional!
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,
}OpCode;

typedef struct
{
    int count;
    int capacity;
    uint8_t * code;
    int* lines;
    ValueArray constants;
}Chunk;

void initChunk(Chunk * chunk);

void writeChunk(Chunk * chunk, uint8_t byte, int line);

void freeChunk(Chunk * chunk);

int addconstant(Chunk* chunk, Value value);


#endif //CHUNK_H
