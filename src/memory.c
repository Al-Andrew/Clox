#include "memory.h"
#include "stdlib.h"
#include "stdio.h"
#include <stdlib.h>

void* reallocate(void* old_ptr, size_t old_size, size_t new_size) {

    void* result = realloc(old_ptr, new_size);

    if(result == NULL) {
        exit(69); // OOM
    }

    return result;
}

void deallocate(void* ptr) {
    free(ptr);
}


#ifdef CLOX_DEBUG_LOG_GC
    #define DEBUG_GC_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif // CLOX_DEBUG_LOG_GC

void Clox_VM_GC(Clox_VM* vm) {

    DEBUG_GC_PRINT("-- GC START");


    DEBUG_GC_PRINT("-- GC END");
}