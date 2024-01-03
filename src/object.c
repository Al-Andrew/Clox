#include "object.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"
#include "vm.h"

Clox_Object* Clox_Object_Allocate(Clox_VM* vm, Clox_Object_Type type, uint32_t size) {
    CLOX_DEV_ASSERT(size >= sizeof(Clox_Object));
    Clox_Object* retval = (Clox_Object*)malloc(size);
    retval->type = type;

    retval->next_object = vm->objects;
    vm->objects = retval; 
    
    return retval;
}

static uint32_t fnv_1a(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

Clox_String* Clox_String_Create(Clox_VM* vm, const char* string, uint32_t len) {
    // Check if the string is already interned
    uint32_t hash = fnv_1a(string, len);
    Clox_Hash_Table_Entry* interned = Clox_Hash_Table_Get_Raw(&vm->strings, string, len, hash);
    if(interned != NULL) {
        return interned->key;
    }

    // Allocate a new one
    Clox_String* retval = (Clox_String*)Clox_Object_Allocate(vm, CLOX_OBJECT_TYPE_STRING, sizeof(Clox_String) + len + 1);
    retval->hash = hash;
    retval->length = len;
    memcpy(retval->characters, string, len);
    retval->characters[len] = '\0';
    Clox_Hash_Table_Set(&vm->strings, retval, CLOX_VALUE_NIL);

    return retval;
}

void Clox_Object_Print(Clox_Object const* const object) {
    switch (object->type) {
        case CLOX_OBJECT_TYPE_STRING: {
            Clox_String* string = (Clox_String*)object;
            printf("\"%.*s\"", string->length, string->characters);
        } break;
    }
}