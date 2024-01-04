#include "value_array.h"
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
        chunk->values = malloc(sizeof(Clox_Value) * chunk->allocated); // TODO(Al-Andrew, AllocFailure): handle
        chunk->values[0] = op;
        chunk->used = 1;
        return;
    }

    if(chunk->used >= chunk->allocated) {
        chunk->allocated *= 2;
        chunk->values = realloc(chunk->values, sizeof(Clox_Value) * chunk->allocated); // TODO(Al-Andrew, AllocFailure): handle
        chunk->values[chunk->used] = op;
        chunk->used += 1;
        return;
    }

    chunk->values[chunk->used] = op;
    chunk->used += 1;
    return;
}

void Clox_Value_Print(Clox_Value value) {
    switch(value.type) {
        case CLOX_VALUE_TYPE_NIL: {
            printf("(nil)");
        } break;
        case CLOX_VALUE_TYPE_BOOL: {
            printf("%s", value.boolean == true? "true": "false");
        } break;
        case CLOX_VALUE_TYPE_NUMBER: {
            printf("%g", value.number);
        } break;
        case CLOX_VALUE_TYPE_OBJECT: {
            Clox_Object_Print(value.object);
        } break;
        default:
            CLOX_UNREACHABLE();
    }
}

bool Clox_Value_Is_Falsy(Clox_Value value) {
    switch (value.type) {

        case CLOX_VALUE_TYPE_NIL: return true;
        case CLOX_VALUE_TYPE_BOOL: return !value.boolean;
        case CLOX_VALUE_TYPE_NUMBER: /* fallthrough */ 
        case CLOX_VALUE_TYPE_OBJECT: {
            return false;
        }
    }
    CLOX_UNREACHABLE();
    return false;
}