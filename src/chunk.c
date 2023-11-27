#include "chunk.h"
#include "common.h"
#include <stdint.h>


Clox_Chunk Clox_Chunk_New_Empty() {
    return (Clox_Chunk){0};
}

void Clox_Chunk_Delete(Clox_Chunk* const chunk) {
    if(chunk->code)
        free(chunk->code);
    Clox_Value_Array_Delete(&chunk->constants);
    *chunk = (Clox_Chunk){0};

    return;
}

void Clox_Chunk_Push(Clox_Chunk* const chunk, uint8_t const data) {
    CLOX_DEV_ASSERT(chunk != NULL);

    if(chunk->code == NULL) {
        chunk->allocated = 1;
        chunk->code = malloc(sizeof(uint8_t) * chunk->allocated); // TODO(Al-Andrew, AllocFailure): handle
        chunk->code[0] = data;
        chunk->used = 1;
        return;
    }

    if(chunk->used >= chunk->allocated) {
        chunk->allocated *= 2;
        chunk->code = realloc(chunk->code, sizeof(uint8_t) * chunk->allocated); // TODO(Al-Andrew, AllocFailure): handle
        chunk->code[chunk->used] = data;
        chunk->used += 1;
        return;
    }

    chunk->code[chunk->used-1] = data;
    chunk->used += 1;
    return;
}

uint32_t Clox_Chunk_Push_Constant(Clox_Chunk* const chunk, Clox_Value const value) {
    Clox_Value_Array_Push_Back(&chunk->constants, value);
    
    return chunk->constants.used - 1;
}

void Clox_Chunk_Print(Clox_Chunk* const chunk, char const* const name) {
    CLOX_DEV_ASSERT(chunk != NULL);
    
    printf("== %s ==\n", name);
    
    for(uint32_t offset = 0; offset < chunk->used;) {
        offset += Clox_Chunk_Print_Op_Code(chunk, offset);
    }

    return;
}

uint32_t Clox_Chunk_Print_Op_Code(Clox_Chunk* const chunk, uint32_t offset) {
    CLOX_DEV_ASSERT(chunk != NULL);
    CLOX_DEV_ASSERT(offset <= chunk->used);

    printf("%04X ", offset);

    Clox_Op_Code opcode = chunk->code[offset];
    switch (opcode) {
        case OP_RETURN: {
            printf("OP_RETURN\n");
            return offset + 1;
        } break;
        case OP_CONSTANT: {
            uint8_t constant_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_CONSTANT", constant_idx);
            Clox_Value_Print(chunk->constants.values[constant_idx]);
            printf("'\n");
            return offset + 2;    
        } break;
        default: {
            printf("Unknown opcode %d\n", (uint32_t)opcode);
            return offset + 1;
        };
    }

    CLOX_UNREACHABLE();
}
