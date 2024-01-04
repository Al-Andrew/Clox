#ifndef CLOX_CHUNK_H_INCLUDED
#define CLOX_CHUNK_H_INCLUDED

#include "common.h"
#include "value.h"
#include <stdint.h>

typedef enum {
    OP_RETURN = 0,
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_ARITHMETIC_NEGATION,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_BOOLEAN_NEGATION,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_PRINT,
    OP_POP,
} Clox_Op_Code;

typedef struct {
    uint32_t used;
    uint32_t allocated;
    uint8_t* code;
    uint32_t* source_lines;
    Clox_Value_Array constants;
} Clox_Chunk;

Clox_Chunk Clox_Chunk_New_Empty();

void Clox_Chunk_Delete(Clox_Chunk* const chunk);

void Clox_Chunk_Push(Clox_Chunk* const chunk, uint8_t const data, uint32_t const source_line);
uint32_t Clox_Chunk_Push_Constant(Clox_Chunk* const chunk, Clox_Value const value); 

void Clox_Chunk_Print(Clox_Chunk* const chunk, char const* const name);
uint32_t Clox_Chunk_Print_Op_Code(Clox_Chunk* const chunk, uint32_t const offset);

#endif // CLOX_COMMON_H_INCLUDED