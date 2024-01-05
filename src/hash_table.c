#include "hash_table.h"
#include <stdint.h>
#include <string.h>
#include "memory.h"

Clox_Hash_Table Clox_Hash_Table_Create() {
    return (Clox_Hash_Table){0};
}

void Clox_Hash_Table_Destory(Clox_Hash_Table* table) {
    // NOTE(Al-Andrew, GC): the keys get cleaned up by the GC?
    if(table->entries) {
        FREE(table->entries, Clox_Hash_Table_Entry, table->allocated);
        table->entries = NULL;
        table->allocated = 0;
        table->used = 0;
    }
}


static Clox_Hash_Table_Entry* Clox_Hash_Table_Find_Entry(Clox_Hash_Table* table, Clox_String* key) {
    uint32_t index = key->hash % table->allocated;

    // NOTE(Al-Andrew): possible infinite loop if our invariant is broken
    for (;;) {
        Clox_Hash_Table_Entry* entry = &table->entries[index];
        Clox_Hash_Table_Entry* tombstone = NULL;

        if (entry->key == NULL) {
            if (CLOX_VALUE_IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) % table->allocated;
    }
}

static Clox_Hash_Table_Entry* Clox_Hash_Table_Find_Entry_In_List(Clox_Hash_Table_Entry* entries, uint32_t count, Clox_String* key) {
    uint32_t index = key->hash % count;

    // NOTE(Al-Andrew): possible infinite loop if our invariant is broken
    for (;;) {
        Clox_Hash_Table_Entry* entry = &entries[index];
        Clox_Hash_Table_Entry* tombstone = NULL;

        if (entry->key == NULL) {
            if (CLOX_VALUE_IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) % count;
    }
}

static void Clox_Hash_Table_Adjust_Capacity(Clox_Hash_Table* table, int new_capacity) {
    Clox_Hash_Table_Entry* entries = ALLOCATE(Clox_Hash_Table_Entry, new_capacity);
    for (int i = 0; i < new_capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = CLOX_VALUE_NIL;
    }

    table->used = 0;
    for (uint32_t i = 0; i < table->allocated; i++) {
        Clox_Hash_Table_Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Clox_Hash_Table_Entry* dest = Clox_Hash_Table_Find_Entry_In_List(entries, new_capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->used++;
    }
    FREE(table->entries, Clox_Hash_Table_Entry, table->allocated);

    table->entries = entries;
    table->allocated = new_capacity;
}

void Clox_Hash_Table_Print(Clox_Hash_Table* table) {
    printf("allocated: %d\n", table->allocated);
    printf("used: %d\n", table->used);
    printf("entries: \n");
    for(unsigned int i = 0; i < table->allocated; ++i) {
        if(table->entries[i].key == NULL) {
            printf("[%03d | 00000000] (Empty)\n", i);
            continue;
        }

        printf("[%03d | %08x] %.*s\n", i, table->entries[i].key->hash, table->entries[i].key->length, table->entries[i].key->characters);
    }
}


bool Clox_Hash_Table_Set(Clox_Hash_Table* table, Clox_String* key, Clox_Value value) {
    if ((table->used + 1) > (table->allocated * CLOX_HASH_TABLE_MAX_LOAD)) {
        uint32_t capacity = table->allocated==0?8:table->allocated * 2;
        Clox_Hash_Table_Adjust_Capacity(table, capacity);
    }

    Clox_Hash_Table_Entry* entry = Clox_Hash_Table_Find_Entry(table, key);
    bool isNewKey = (entry->key == NULL);
    if (isNewKey && CLOX_VALUE_IS_NIL(entry->value)) {
        table->used++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void Clox_Hash_Table_Set_All(Clox_Hash_Table* from, Clox_Hash_Table* to) {
    for (uint32_t i = 0; i < from->allocated; i++) {
        Clox_Hash_Table_Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            Clox_Hash_Table_Set(to, entry->key, entry->value);
        }
    }
}

bool Clox_Hash_Table_Get(Clox_Hash_Table* table, Clox_String* key, Clox_Value* value) {
    if (table->used == 0) {
        return false;
    }

    Clox_Hash_Table_Entry* entry = Clox_Hash_Table_Find_Entry(table, key);
    if (entry->key == NULL) {
        return false;
    }

    *value = entry->value;
    return true;
}

Clox_Hash_Table_Entry* Clox_Hash_Table_Get_Raw(Clox_Hash_Table* table, char const*const string, uint32_t const len, uint32_t const hash) {
    if (table->used == 0) return NULL;

    uint32_t index = hash % table->allocated;
    // NOTE(Al-Andrew): possible infinite loop if our invariant is broken
    for (;;) {
        Clox_Hash_Table_Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (CLOX_VALUE_IS_NIL(entry->value)) {
                return NULL;
            }
        } else if (entry->key->length == len && entry->key->hash == hash) {
            // We have a promising candidate
            int cmp_result = memcmp(entry->key->characters, string, len);
            if (cmp_result == 0) {
                // We found it.
                return entry;
            }
        }

        index = (index + 1) % table->allocated;
    }
}

bool Clox_Hash_Table_Remove(Clox_Hash_Table* table, Clox_String* key) {
    if (table->used == 0) { 
        return false;
    }

    Clox_Hash_Table_Entry* entry = Clox_Hash_Table_Find_Entry(table, key);
    if (entry->key == NULL) {
        return false;
    }

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = CLOX_VALUE_BOOL(true);
    return true;
}

