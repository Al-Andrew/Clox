#ifndef CLOX_VALUE_ARRAY_H_INCLUDED
#define CLOX_VALUE_ARRAY_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef struct Clox_Value Clox_Value;

#ifndef CLOX_VALUE_DEFINED
struct Clox_Value;
#endif // CLOX_VALUE_DEFINED

typedef struct {
  uint32_t used;
  uint32_t allocated;
  Clox_Value* values;
} Clox_Value_Array;


Clox_Value_Array Clox_Value_Array_New_Empty();

void Clox_Value_Array_Delete(Clox_Value_Array* const array);

void Clox_Value_Array_Push_Back(Clox_Value_Array* const array, Clox_Value const op);
void Clox_Value_Print(Clox_Value value);

bool Clox_Value_Is_Falsy(Clox_Value value);

#endif // CLOX_VALUE_ARRAY_H_INCLUDED