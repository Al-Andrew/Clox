#include "chunk.h"
#include "common.h"
#include "value.h"
#include "object.h"
#include "memory.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

Clox_Chunk Clox_Chunk_New_Empty() {
    return (Clox_Chunk){0};
}

void Clox_Chunk_Delete(Clox_Chunk* const chunk) {
    if(chunk == NULL) {
        return;
    }
    
    if(chunk->code) {
        FREE(chunk->code, uint8_t, chunk->allocated);
    }
    if(chunk->source_lines) {
        FREE(chunk->source_lines, uint32_t, chunk->allocated);
    }
    Clox_Value_Array_Delete(&chunk->constants);
    *chunk = (Clox_Chunk){0};

    return;
}

void Clox_Chunk_Push(Clox_Chunk* const chunk, uint8_t const data, uint32_t const source_line) {
    CLOX_DEV_ASSERT(chunk != NULL);

    if(chunk->used >= chunk->allocated) {
        size_t old_allocated = chunk->allocated;
        chunk->allocated = (chunk->allocated == 0)?(8):(chunk->allocated*2);
        
        uint8_t* new_code = ALLOCATE(uint8_t, chunk->allocated);
        memcpy(new_code, chunk->code, old_allocated);
        if(chunk->code) {
            FREE(chunk->code, uint8_t, old_allocated);
        }
        chunk->code = new_code;

        uint32_t* new_lines = ALLOCATE(uint32_t, chunk->allocated);
        memcpy(new_lines, chunk->source_lines, old_allocated);
        if(chunk->source_lines) {
            FREE(chunk->source_lines, uint8_t, old_allocated);
        }
        chunk->source_lines = new_lines;
        
        chunk->code[chunk->used] = data;
        chunk->source_lines[chunk->used] = source_line;
        chunk->used += 1;
        return;
    }

    chunk->code[chunk->used] = data;
    chunk->source_lines[chunk->used] = source_line;
    chunk->used += 1;
    return;
}

uint32_t Clox_Chunk_Push_Constant(Clox_Chunk* const chunk, Clox_Value const value) {
    Clox_VM* vm = the_vm; // FIXME(Al-Andrew): hack

    Clox_VM_Stack_Push(vm, value);
    Clox_Value_Array_Push_Back(&chunk->constants, value);
    Clox_VM_Stack_Pop(vm);
    return chunk->constants.used - 1;
}

void Clox_Chunk_Print(Clox_Chunk* const chunk, char const* const name) {
    CLOX_DEV_ASSERT(chunk != NULL);
    
    printf("== BEGIN %s ==\n", name);
    
    for(uint32_t offset = 0; offset < chunk->used;) {
        offset = Clox_Chunk_Print_Op_Code(chunk, offset);
    }

    printf("== END   %s ==\n", name);
    return;
}

uint32_t Clox_Chunk_Print_Op_Code(Clox_Chunk* const chunk, uint32_t const offset) {
    CLOX_DEV_ASSERT(chunk != NULL);
    CLOX_DEV_ASSERT(offset <= chunk->used);

    printf("%04X ", offset);
    if (offset > 0
        && chunk->source_lines[offset] == chunk->source_lines[offset - 1])
    {
        printf("   | ");
    } else {
        printf("%4d ", chunk->source_lines[offset]);
    }

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
        case OP_NIL: {
            printf("OP_NIL\n");
            return offset + 1;
        } break;
        case OP_TRUE: {
            printf("OP_TRUE\n");
            return offset + 1;
        } break;
        case OP_FALSE: {
            printf("OP_FALSE\n");
            return offset + 1;
        } break;
        case OP_ARITHMETIC_NEGATION: {
            printf("OP_ARITHMETIC_NEGATION\n");
            return offset + 1;
        } break;
        case OP_BOOLEAN_NEGATION: {
            printf("OP_BOOLEAN_NEGATION\n");
            return offset + 1;
        } break;
        case OP_ADD: {
            printf("OP_ADD\n");
            return offset + 1;
        } break;
        case OP_SUB: {
            printf("OP_SUB\n");
            return offset + 1;
        } break;
        case OP_MUL: {
            printf("OP_MUL\n");
            return offset + 1;
        } break;
        case OP_DIV: {
            printf("OP_DIV\n");
            return offset + 1;
        } break;
        case OP_LESS: {
            printf("OP_LESS\n");
            return offset + 1;
        } break;
        case OP_GREATER: {
            printf("OP_GREATER\n");
            return offset + 1;
        } break;
        case OP_EQUAL: {
            printf("OP_EQUAL\n");
            return offset + 1;
        } break;
        case OP_PRINT: {
            printf("OP_PRINT\n");
            return offset + 1;
        } break;
        case OP_POP: {
            printf("OP_POP\n");
            return offset + 1;
        } break;
        case OP_DEFINE_GLOBAL: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_DEFINE_GLOBAL", var_name_idx);
            Clox_Value_Print(chunk->constants.values[var_name_idx]);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_GET_GLOBAL: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_GET_GLOBAL", var_name_idx);
            Clox_Value_Print(chunk->constants.values[var_name_idx]);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_SET_GLOBAL: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_SET_GLOBAL", var_name_idx);
            Clox_Value_Print(chunk->constants.values[var_name_idx]);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_GET_LOCAL: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_GET_LOCAL", var_name_idx);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_SET_LOCAL: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_SET_LOCAL", var_name_idx);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_JUMP_IF_FALSE: {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            int sign = 1;
            uint32_t target = offset + 3 + sign * jump;
            printf("%-16s %4d -> %04X\n", "OP_JUMP_IF_FALSE", offset, target);
            return offset + 3;
        } break;
        case OP_JUMP: {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            int sign = 1;
            uint32_t target = offset + 3 + sign * jump;
            printf("%-16s %4d -> %04X\n", "OP_JUMP", offset, target);
            return offset + 3;
        } break;
        case OP_LOOP: {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            int sign = -1;
            uint32_t target = offset + 3 + sign * jump;
            printf("%-16s %4d -> %04X\n", "OP_LOOP", offset, target);
            return offset + 3;
        } break;
        case OP_CALL: {
            uint8_t argc = chunk->code[offset + 1];
            printf("%-16s argc: %4d\n", "OP_CALL", argc);
            return offset + 2;
        } break;
            case OP_CLOSURE: {
            uint8_t constant = chunk->code[offset + 1];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            Clox_Value_Print(chunk->constants.values[constant]);
            printf("\n");

            Clox_Function* function = (Clox_Function*)(chunk->constants.values[constant].object);
            for (int j = 0; j < function->upvalue_count; j++) {
                int isLocal = chunk->code[offset + j*2];
                int index = chunk->code[offset + j*2 + 1];
                printf("%04d    |                     %s %d\n", offset + j , isLocal ? "local" : "upvalue", index);
            }

            return offset + 2 + function->upvalue_count*2;
        } break;
        case OP_GET_UPVALUE: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_GET_UPVALUE", var_name_idx);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_SET_UPVALUE: {
            uint8_t var_name_idx = chunk->code[offset + 1];
            printf("%-16s %4d '", "OP_SET_UPVALUE", var_name_idx);
            printf("'\n");
            return offset + 2;
        } break;
        case OP_CLOSE_UPVALUE: {
            printf("OP_CLOSE_UPVALUE\n");
            return offset + 1;
        } break;
        default: {
            printf("Unknown opcode %d\n", (uint32_t)opcode);
            return offset + 1;
        };
    }

    CLOX_UNREACHABLE();
}
