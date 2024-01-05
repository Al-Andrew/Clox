#include "memory.h"
#include "stdlib.h"
#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "compiler.h"
#include "value_array.h"

void* reallocate_(void* old_ptr, size_t old_size, size_t new_size) {
    Clox_VM* vm = the_vm; // FIXME(Al-Andrew): hack
    Clox_Parser* parser = the_parser;
    
    vm->bytes_allocated += new_size - old_size;

    if(new_size > old_size) {

#ifdef CLOX_DEBUG_STRESS_GC
        Clox_VM_GC(vm, the_parser);
#endif
        if (vm->bytes_allocated > vm->next_GC) {
            Clox_VM_GC(vm, parser);
        }
    }

    if(new_size == 0) {
        free(old_ptr);
        return NULL;
    }

    void* result = realloc(old_ptr, new_size);

    if(result == NULL) {
        exit(69); // OOM
    }

    return result;
}

void Clox_VM_GC_Mark_Object(Clox_Object* object) {
    if (object == NULL) return;
    if (object->is_marked) return;
    object->is_marked = true;
#ifdef CLOX_DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    Clox_Value_Print(CLOX_VALUE_OBJECT(object));
    printf("\n");
#endif

    Clox_VM* vm = the_vm; // FIXME(Al-Andrew): hack;


    if (vm->gray_capacity < vm->gray_count + 1) {
        vm->gray_capacity = vm->gray_capacity == 0?8:vm->gray_capacity*2;
        vm->gray_stack = (Clox_Object**)realloc(vm->gray_stack, sizeof(Clox_Object*) * vm->gray_capacity); // NOTE(AAL): how unfunny would it be if we used reallocate(...) here?
    }
    if (vm->gray_stack == NULL) exit(66); // WE BAD
    vm->gray_stack[vm->gray_count++] = object;
}

static void Clox_VM_GC_Mark_Value(Clox_Value* value) {
    if (value->type == CLOX_VALUE_TYPE_OBJECT) {
        Clox_VM_GC_Mark_Object(value->object);
    }
}

static void Clox_VM_GC_Mark_Hash_Table(Clox_Hash_Table* table) {
    for (uint32_t i = 0; i < table->allocated; i++) {
        Clox_Hash_Table_Entry* entry = &table->entries[i];
        Clox_VM_GC_Mark_Object((Clox_Object*)entry->key);
        Clox_VM_GC_Mark_Value(&entry->value);
    }
}

static void Clox_VM_GC_Mark_Roots(Clox_VM* vm, Clox_Parser* parser) {
    for (Clox_Value* slot = vm->stack; slot < vm->stack_top; slot++) {
        Clox_VM_GC_Mark_Value(slot);
    }
    Clox_VM_GC_Mark_Hash_Table(&vm->globals);
    Clox_VM_GC_Mark_Compiler_Roots(parser);

    for (int i = 0; i < vm->call_frame_count; i++) {
        Clox_VM_GC_Mark_Object((Clox_Object*)vm->frames[i].closure);
    }

    for (Clox_UpvalueObj* upvalue = vm->open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        Clox_VM_GC_Mark_Object((Clox_Object*)upvalue);
    }
}

static void Clox_VM_GC_MARK_Array(Clox_Value_Array* array) {
    for (uint32_t i = 0; i < array->used; i++) {
        Clox_VM_GC_Mark_Value(&array->values[i]);
    }
}

static void Clox_VM_GC_Blacken_Object(Clox_VM* vm, Clox_Object* object) {
#ifdef CLOX_DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    fflush(0);
    Clox_Value_Print(CLOX_VALUE_OBJECT(object));
    printf("\n");
#endif

    switch (object->type) {
        case CLOX_OBJECT_TYPE_NATIVE: /* fallthrough */
        case CLOX_OBJECT_TYPE_STRING: {
            /* nothing to do here */
        } break;
        case CLOX_OBJECT_TYPE_FUNCTION: {
            Clox_Function* function = (Clox_Function*)object;
            Clox_VM_GC_Mark_Object((Clox_Object*)function->name);
            Clox_VM_GC_MARK_Array(&function->chunk.constants);
        } break;
        case CLOX_OBJECT_TYPE_CLOSURE: {
            Clox_Closure* closure = (Clox_Closure*)object;
            Clox_VM_GC_Mark_Object((Clox_Object*)closure->function);
            for (int i = 0; i < closure->upvalue_count; i++) {
                Clox_VM_GC_Mark_Object((Clox_Object*)closure->upvalues[i]);
            }
        } break;
        case CLOX_OBJECT_TYPE_UPVALUE: {
            Clox_VM_GC_Mark_Value(&((Clox_UpvalueObj*)object)->closed);
        } break;
    }
}

static void Clox_VM_GC_Trace_Reference(Clox_VM* vm, Clox_Parser* parser) {
    while (vm->gray_count > 0) {
        Clox_Object* object = vm->gray_stack[--vm->gray_count];
        Clox_VM_GC_Blacken_Object(vm, object);
    }
}

static void Clox_VM_GC_Sweep(Clox_VM* vm, Clox_Parser* parser) {
    Clox_Object* previous = NULL;
    Clox_Object* object = vm->objects;
    while (object != NULL) {
        if (object->is_marked) {
            object->is_marked = false;
            previous = object;
            object = object->next_object;
        } else {
            Clox_Object* unreached = object;
            object = object->next_object;
            if (previous != NULL) {
                previous->next_object = object;
            } else {
                vm->objects = object;
            }

            Clox_Object_Deallocate(vm, unreached);
        }
    }
}

static void Clox_VM_GC_Hash_Table_Remove_White(Clox_Hash_Table* table) {
    for (uint32_t i = 0; i < table->allocated; i++) {
        Clox_Hash_Table_Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.is_marked) {
            Clox_Hash_Table_Remove(table, entry->key);
        }
    }
}

void Clox_VM_GC(Clox_VM* vm, Clox_Parser* parser) {

    DEBUG_GC_PRINT("-- GC START\n");\
    size_t before = vm->bytes_allocated;

    Clox_VM_GC_Mark_Roots(vm, parser);
    Clox_VM_GC_Trace_Reference(vm, parser);
    Clox_VM_GC_Hash_Table_Remove_White(&vm->strings);
    Clox_VM_GC_Sweep(vm, parser);
    vm->next_GC = vm->bytes_allocated * CLOX_GC_HEAP_GROW_FACTOR;

    DEBUG_GC_PRINT("-- GC END\n");
    size_t after = vm->bytes_allocated;
    size_t diff = before - after;
    DEBUG_GC_PRINT("   collected %zu bytes (from %zu to %zu) next at %zu\n", diff, before, after, vm->next_GC);
}