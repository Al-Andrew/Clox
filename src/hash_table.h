#ifndef CLOX_HASH_TABLE_H_INCLUDED
#define CLOX_HASH_TABLE_H_INCLUDED

#include "value.h"
#include <stdint.h>
#include "object.h"

typedef struct Clox_Hash_Table_Entry Clox_Hash_Table_Entry;
struct Clox_Hash_Table_Entry {
    Clox_String* key;
    Clox_Value value;
};



#define CLOX_HASH_TABLE_MAX_LOAD 0.75

typedef struct Clox_Hash_Table Clox_Hash_Table;
struct Clox_Hash_Table {
    uint32_t used;
    uint32_t allocated;
    Clox_Hash_Table_Entry* entries;
};

Clox_Hash_Table Clox_Hash_Table_Create();
void Clox_Hash_Table_Destory(Clox_Hash_Table* table);
bool Clox_Hash_Table_Set(Clox_Hash_Table* table, Clox_String* key, Clox_Value value);
void Clox_Hash_Table_Set_All(Clox_Hash_Table* from, Clox_Hash_Table* to);
bool Clox_Hash_Table_Get(Clox_Hash_Table* table, Clox_String* key, Clox_Value* value);
Clox_Hash_Table_Entry* Clox_Hash_Table_Get_Raw(Clox_Hash_Table* table, char const*const string, uint32_t const len, uint32_t const hash);
bool Clox_Hash_Table_Remove(Clox_Hash_Table* table, Clox_String* key);
void Clox_Hash_Table_Print(Clox_Hash_Table* table);


#endif // CLOX_HASH_TABLE_H_INCLUDED