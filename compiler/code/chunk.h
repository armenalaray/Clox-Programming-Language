/* date = February 21st 2025 4:48 pm */

#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
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
