#ifndef CLOX_OBJECT_H_INCLUDED
#define CLOX_OBJECT_H_INCLUDED

#include <stdint.h>

typedef struct Clox_VM Clox_VM;
struct Clox_VM;

typedef enum {
    CLOX_OBJECT_TYPE_STRING,
} Clox_Object_Type;

typedef struct Clox_Object Clox_Object;
struct Clox_Object {
    Clox_Object_Type type;
    Clox_Object* next_object;
};


Clox_Object* Clox_Object_Allocate(Clox_VM* vm, Clox_Object_Type type, uint32_t size);
void Clox_Object_Print(Clox_Object const* const object);

typedef struct Clox_String Clox_String;
struct Clox_String {
    Clox_Object obj;
    uint32_t hash;
    uint32_t length;
    char characters[0];
};

Clox_String* Clox_String_Create(Clox_VM* vm, const char* string, uint32_t len);

#endif // CLOX_OBJECT_H_INCLUDED