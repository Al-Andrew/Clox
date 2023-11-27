#include "value.h"


Clox_Value_Array Clox_Value_Array_New_Empty() {
    return (Clox_Value_Array){0};
}

void Clox_Value_Array_Delete(Clox_Value_Array* const chunk) {
    if(chunk->values)
        free(chunk->values);
    *chunk = (Clox_Value_Array){0};

    return;
}

void Clox_Value_Array_Push_Back(Clox_Value_Array* const chunk, Clox_Value const op) {
    CLOX_DEV_ASSERT(chunk != NULL);

    if(chunk->values == NULL) {
        chunk->allocated = 8;
        chunk->values = malloc(sizeof(uint8_t) * chunk->allocated); // TODO(Al-Andrew, AllocFailure): handle
        chunk->values[0] = op;
        chunk->used = 1;
        return;
    }

    if(chunk->used >= chunk->allocated) {
        chunk->allocated *= 2;
        chunk->values = realloc(chunk->values, sizeof(uint8_t) * chunk->allocated); // TODO(Al-Andrew, AllocFailure): handle
        chunk->values[chunk->used] = op;
        chunk->used += 1;
        return;
    }

    chunk->values[chunk->used-1] = op;
    chunk->used += 1;
    return;
}

void Clox_Value_Print(Clox_Value value) {
    printf("%g", (double)value);
}