#ifndef CLOX_COMPILER_H_INCLUDED
#define CLOX_COMPILER_H_INCLUDED

#include "chunk.h"
#include "value.h"

Clox_Function* Clox_Compile_Source_To_Function(Clox_VM* vm, const char* source);

#endif // CLOX_COMPILER_H_INCLUDED