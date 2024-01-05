#ifndef CLOX_COMPILER_H_INCLUDED
#define CLOX_COMPILER_H_INCLUDED

#include "chunk.h"
#include "value.h"
#include "object.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"
#include <stdint.h>
#include <string.h>
#include "memory.h"

#define CLOX_DEBUG_PRINT_COMPILED_CHUNKS


typedef struct {
    Clox_Token name;
    int depth;
    bool is_captured;
} Clox_Local;

typedef enum {
    CLOX_FUNCTION_TYPE_FUNCTION,
    CLOX_FUNCTION_TYPE_SCRIPT
} Clox_Function_Type;

typedef struct {
    uint8_t index;
    bool isLocal;
} Clox_Upvalue;

typedef struct Clox_Compiler Clox_Compiler;
struct Clox_Compiler {
    Clox_Compiler* enclosing;
    Clox_Function* function;
    Clox_Function_Type type;
    Clox_Local locals[UINT8_MAX + 1];
    Clox_Upvalue upvalues[UINT8_MAX + 1];
    int localCount;
    int scopeDepth;
};

struct Clox_Parser {
    Clox_Token current;
    Clox_Token previous;
    Clox_Scanner* scanner;
    Clox_VM* vm;
    Clox_Compiler* compiler;
    bool had_error;
    bool panic_mode;
};

Clox_Function* Clox_Compile_Source_To_Function(Clox_VM* vm, const char* source);

void Clox_VM_GC_Mark_Compiler_Roots(Clox_Parser* parser);


#endif // CLOX_COMPILER_H_INCLUDED