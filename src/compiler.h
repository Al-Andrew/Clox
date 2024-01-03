#ifndef CLOX_COMPILER_H_INCLUDED
#define CLOX_COMPILER_H_INCLUDED

#include "chunk.h"

bool Clox_Compile_Source_To_Chunk(Clox_VM* vm, const char* source, Clox_Chunk* chunk);

#endif // CLOX_COMPILER_H_INCLUDED