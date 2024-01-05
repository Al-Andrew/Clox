#ifndef CLOX_OBJECT_H_INCLUDED
#define CLOX_OBJECT_H_INCLUDED

#include <stdint.h>
#include "chunk.h"
#include "value.h"

typedef struct Clox_VM Clox_VM;
struct Clox_VM;

typedef enum {
    CLOX_OBJECT_TYPE_STRING,
    CLOX_OBJECT_TYPE_FUNCTION,
    CLOX_OBJECT_TYPE_NATIVE,
    CLOX_OBJECT_TYPE_CLOSURE,
    CLOX_OBJECT_TYPE_UPVALUE,
} Clox_Object_Type;

typedef struct Clox_Object Clox_Object;
struct Clox_Object {
    Clox_Object_Type type;
    Clox_Object* next_object;
};


Clox_Object* Clox_Object_Allocate(Clox_VM* vm, Clox_Object_Type type, uint32_t size);
void Clox_Object_Deallocate(Clox_VM* vm, Clox_Object* object);
void Clox_Object_Print(Clox_Object const* const object);


typedef Clox_Value (*Clox_Native_Fn)(int argCount, Clox_Value* args);

typedef struct {
    Clox_Object obj;
    Clox_Native_Fn function;
} Clox_Native;

Clox_Native* Clox_Native_Create(Clox_VM* vm, Clox_Native_Fn lambda);

typedef struct Clox_String Clox_String;
struct Clox_String {
    Clox_Object obj;
    uint32_t hash;
    uint32_t length;
    char characters[0];
};

Clox_String* Clox_String_Create(Clox_VM* vm, const char* string, uint32_t len);

typedef struct Clox_Function Clox_Function;
struct Clox_Function {
    Clox_Object obj;
    int arity;
    int upvalue_count;
    Clox_Chunk chunk;
    Clox_String* name;
};


Clox_Function* Clox_Function_Create_Empty(Clox_VM* vm);


typedef struct Clox_UpvalueObj Clox_UpvalueObj;
struct Clox_UpvalueObj {
    Clox_Object obj;
    Clox_Value* location;
    Clox_UpvalueObj* next;
    Clox_Value closed;
};

Clox_UpvalueObj* Clox_UpvalueObj_Create(Clox_VM* vm, Clox_Value* slot);

typedef struct {
    Clox_Object obj;
    Clox_Function* function;
    int upvalue_count;
    Clox_UpvalueObj* upvalues[];
} Clox_Closure;

Clox_Closure* Clox_Closure_Create(Clox_VM* vm, Clox_Function* function);
Clox_UpvalueObj* Clox_Closure_Capture_Upvalue(Clox_VM* vm, Clox_Value* value);


#endif // CLOX_OBJECT_H_INCLUDED