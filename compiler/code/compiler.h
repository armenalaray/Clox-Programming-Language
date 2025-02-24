/* date = February 22nd 2025 9:37 pm */

#ifndef clox_compiler_h
#define clox_compiler_h

#include "scanner.h"
#include "chunk.h"

bool compile(const char* source, Chunk* chunk);

#endif //COMPILER_H
