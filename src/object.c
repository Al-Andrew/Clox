#include "object.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include "vm.h"

Clox_Object* Clox_Object_Allocate(Clox_VM* vm, Clox_Object_Type type, uint32_t size) {
    CLOX_DEV_ASSERT(size >= sizeof(Clox_Object));
    Clox_Object* retval = (Clox_Object*)malloc(size);
    retval->type = type;

    retval->next_object = vm->objects;
    vm->objects = retval; 
    
    return retval;
}

Clox_String* Clox_String_Create(Clox_VM* vm, const char* string, uint32_t len) {
    Clox_String* retval = (Clox_String*)Clox_Object_Allocate(vm, CLOX_OBJECT_TYPE_STRING, sizeof(Clox_String) + len + 1);
    retval->length= len;
    memcpy(retval->characters, string, len);
    retval->characters[len] = '\0';

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