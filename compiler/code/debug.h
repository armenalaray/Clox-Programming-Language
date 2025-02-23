/* date = February 21st 2025 7:25 pm */

#ifndef DEBUG_H
#define DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk * chunk, const char * name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif //DEBUG_H
