#ifndef CLOX_VM_H_INCLUDED
#define CLOX_VM_H_INCLUDED

// NOTE(Al-Andrew): uncomment for debugging 
#define CLOX_DEBUG_TRACE_EXECUTION
#define CLOX_DEBUG_TRACE_STACK

#include "chunk.h"
#include "value.h"
#include "object.h"
#include "hash_table.h"

#define CLOX_MAX_CALL_FRAMES 64
#define CLOX_MAX_STACK (CLOX_MAX_CALL_FRAMES * (UINT8_MAX + 1))

typedef struct {
  Clox_Closure* closure;
  uint8_t* instruction_pointer;
  Clox_Value* slots;
} Clox_Call_Frame;

struct Clox_VM{
  Clox_Chunk* chunk;
  uint8_t* instruction_pointer;
  Clox_Call_Frame frames[CLOX_MAX_CALL_FRAMES];
  int call_frame_count;
  Clox_Value stack[CLOX_MAX_STACK];
  Clox_Value* stack_top;
  Clox_Object* objects;
  Clox_Hash_Table strings;
  Clox_Hash_Table globals;
  Clox_UpvalueObj* open_upvalues;
  int gray_count;
  int gray_capacity;
  Clox_Object** gray_stack;
  size_t bytes_allocated;
  size_t next_GC;
};


typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} Clox_Interpret_Status;

typedef struct {
    Clox_Interpret_Status status;
    char const * message;
    Clox_Value  return_value;
} Clox_Interpret_Result;

void Clox_VM_Init(Clox_VM* vm);

void Clox_VM_Stack_Push(Clox_VM* const vm, Clox_Value const value);
Clox_Value Clox_VM_Stack_Pop(Clox_VM* const vm);


void Clox_VM_Delete(Clox_VM* const vm);

Clox_Interpret_Result Clox_VM_Interpret_Chunk(Clox_VM* const vm, Clox_Chunk* const chunk);
Clox_Interpret_Result Clox_VM_Interpret_Source(Clox_VM* const vm, const char* source);
void Clox_VM_Define_Native(Clox_VM* vm, const char* name, Clox_Native_Fn function);

#endif // CLOX_VM_H_INCLUDED