#ifndef CLOX_VALUE_H_INCLUDED
#define CLOX_VALUE_H_INCLUDED

#include "common.h"
#include "object.h"

typedef enum {
  CLOX_VALUE_TYPE_NIL,
  CLOX_VALUE_TYPE_BOOL,
  CLOX_VALUE_TYPE_NUMBER,
  CLOX_VALUE_TYPE_OBJECT,
} Clox_Value_Type;


#ifndef CLOX_VALUE_DEFINED
struct Clox_Value {
  Clox_Value_Type type;
  union {
    bool boolean;
    double number;
    Clox_Object* object;
  }; 
};

#define CLOX_VALUE_DEFINED
#endif // CLOX_VALUE_DEFINED

#define CLOX_VALUE_IS_BOOL(value)    ((value).type == CLOX_VALUE_TYPE_BOOL)
#define CLOX_VALUE_IS_NIL(value)     ((value).type == CLOX_VALUE_TYPE_NIL)
#define CLOX_VALUE_IS_NUMBER(value)  ((value).type == CLOX_VALUE_TYPE_NUMBER)
#define CLOX_VALUE_IS_OBJECT(value)  ((value).type == CLOX_VALUE_TYPE_OBJECT)

// NOTE(Al-Andrew): these ones require two checks (and thus can't be macros):
static inline bool CLOX_VALUE_IS_STRING(Clox_Value value) {
  return (CLOX_VALUE_IS_OBJECT(value) && (value).object->type == CLOX_OBJECT_TYPE_STRING);
}

#define CLOX_VALUE_BOOL(value)   ((Clox_Value){CLOX_VALUE_TYPE_BOOL, {.boolean = value}})
#define CLOX_VALUE_NIL           ((Clox_Value){CLOX_VALUE_TYPE_NIL, {.number = 0}})
#define CLOX_VALUE_NUMBER(value) ((Clox_Value){CLOX_VALUE_TYPE_NUMBER, {.number = value}})
#define CLOX_VALUE_OBJECT(obj)   ((Clox_Value){CLOX_VALUE_TYPE_OBJECT, {.object = (Clox_Object*)(obj)}})

#endif // CLOX_VALUE_H_INCLUDED