#ifndef CLOX_VM_H_INCLUDED
#define CLOX_VM_H_INCLUDED

// NOTE(Al-Andrew): uncomment for debugging 
#define CLOX_DEBUG_TRACE_EXECUTION
#define CLOX_DEBUG_TRACE_STACK

#include "chunk.h"
#include "value.h"
#include "hash_table.h"

#define CLOX_MAX_STACK 512

struct Clox_VM{
  Clox_Chunk* chunk;
  uint8_t* instruction_pointer;
  Clox_Value stack[CLOX_MAX_STACK];
  Clox_Value* stack_top;
  Clox_Object* objects;
  Clox_Hash_Table strings;
};


typedef enum {
  INTERPRET_COMPILE_ERROR = 0,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_OK,
} Clox_Interpret_Status;

typedef struct {
    Clox_Interpret_Status status;
    Clox_Value  return_value;
} Clox_Interpret_Result;

Clox_VM Clox_VM_New_Empty();

void Clox_VM_Delete(Clox_VM* const vm);

Clox_Interpret_Result Clox_VM_Interpret_Chunk(Clox_VM* const vm, Clox_Chunk* const chunk);
Clox_Interpret_Result Clox_VM_Interpret_Source(Clox_VM* const vm, const char* source);

#endif // CLOX_VM_H_INCLUDED