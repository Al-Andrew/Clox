#ifndef CLOX_VALUE_H_INCLUDED
#define CLOX_VALUE_H_INCLUDED

#include "common.h"

typedef double Clox_Value;


typedef struct {
  uint32_t used;
  uint32_t allocated;
  Clox_Value* values;
} Clox_Value_Array;


Clox_Value_Array Clox_Value_Array_New_Empty();

void Clox_Value_Array_Delete(Clox_Value_Array* const array);

void Clox_Value_Array_Push_Back(Clox_Value_Array* const array, Clox_Value const op);
void Clox_Value_Print(Clox_Value value);


#endif // CLOX_VALUE_H_INCLUDED