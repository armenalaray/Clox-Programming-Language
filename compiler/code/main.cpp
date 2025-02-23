

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
    initVM();
    
    Chunk chunk;
    initChunk(&chunk);
    
    /*load constant from address*/
    int index = addconstant(&chunk,1.2);
    /*tienes una manera de acceder a la informacion desde otro lado*/
    writeChunk(&chunk, OP_CONSTANT, 123); 
    writeChunk(&chunk, (uint8_t)index, 123);
    
    index = addconstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123); 
    writeChunk(&chunk, (uint8_t)index, 123);
    
    writeChunk(&chunk, OP_ADD, 123);
    
    index = addconstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123); 
    writeChunk(&chunk, (uint8_t)index, 123);
    
    writeChunk(&chunk, OP_DIVIDE, 123);
    
    writeChunk(&chunk, OP_NEGATE, 123);
    writeChunk(&chunk, OP_RETURN, 123);
    
    disassembleChunk(&chunk, "test chunk");
    
    interpret(&chunk);
    
    freeChunk(&chunk);
    
    freeVM();
    return 0;
}

