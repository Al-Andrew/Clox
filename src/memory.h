#ifndef CLOX_MEMORY_H_INCLUDED
#define CLOX_MEMORY_H_INCLUDED

#include "vm.h"

#define ALLOCATE(type, count) (type*)reallocate_(NULL, 0, sizeof(type) * (count))

#define FREE(pointer, type, count) reallocate_(pointer, sizeof(type) * count, 0)

#define CLOX_DEBUG_STRESS_GC
#define CLOX_DEBUG_LOG_GC

#define CLOX_GC_HEAP_GROW_FACTOR 2

#ifdef CLOX_DEBUG_LOG_GC
    #define DEBUG_GC_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_GC_PRINT(fmt, ...) 
#endif // CLOX_DEBUG_LOG_GC

extern Clox_VM* the_vm;

typedef struct Clox_Parser Clox_Parser;
struct Clox_Parser;

extern Clox_Parser* the_parser;

void* reallocate_(void* old_ptr, size_t old_size, size_t new_size);

void Clox_VM_GC_Mark_Object(Clox_Object* object);
void Clox_VM_GC(Clox_VM* vm, Clox_Parser* parser);


#endif // CLOX_MEMORY_H_INCLUDED