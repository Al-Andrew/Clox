#ifndef CLOX_MEMORY_H_INCLUDED
#define CLOX_MEMORY_H_INCLUDED

#include "vm.h"

#define CLOX_DEBUG_STRESS_GC
#define CLOX_DEBUG_LOG_GC

void* reallocate(void* old_ptr, size_t old_size, size_t new_size);
void deallocate(void* ptr);

void Clox_VM_GC(Clox_VM* vm);


#endif // CLOX_MEMORY_H_INCLUDED