#include "vm.h"
#include "common.h"
#include "compiler.h"
#include <float.h>
#include <string.h>

Clox_VM Clox_VM_New_Empty() {
    return (Clox_VM){0};
}

void Clox_VM_Delete(Clox_VM* const vm) {
    // NOTE(Al-Andrew, Leak): do we own the chunk?

    Clox_Hash_Table_Destory(&vm->strings);
    Clox_Hash_Table_Destory(&vm->globals);
    Clox_Object* it = vm->objects;
    while(it != NULL) {
        Clox_Object* next = it->next_object;
        free(it);
        it = next;
    }
}

static inline void Clox_VM_Stack_Push(Clox_VM* const vm, Clox_Value const value) {
    *(vm->stack_top++) = value;
}

static inline Clox_Value Clox_VM_Stack_Pop(Clox_VM* const vm) {
    return *(--vm->stack_top);
}

static inline Clox_Value Clox_VM_Stack_Peek(Clox_VM* const vm, uint32_t depth) {
    return *(vm->stack_top - 1 - depth);
}


// NOTE(Al-Andrew): assumes `Clox_VM* const vm` is in scope and we're returning Clox_Interpret_Result
#define CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(N) { if((vm->stack_top - vm->stack) < N) { return (Clox_Interpret_Result){.status = INTERPRET_COMPILE_ERROR}; } }
// TODO(Al-Andrew, Diagnostics): better diagnostics 
#define CLOX_VM_ASSURE_STACK_TYPE_0(T) { if(Clox_VM_Stack_Peek(vm, 0).type != T) { return (Clox_Interpret_Result){.status = INTERPRET_RUNTIME_ERROR}; } }
#define CLOX_VM_ASSURE_STACK_TYPE_1(T) { if(Clox_VM_Stack_Peek(vm, 1).type != T) { return (Clox_Interpret_Result){.status = INTERPRET_RUNTIME_ERROR}; } }

static Clox_Interpret_Result Clox_VM_Runtime_Error(Clox_VM* vm, char const* const fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);

    return (Clox_Interpret_Result){.status = INTERPRET_RUNTIME_ERROR};
}

Clox_Interpret_Result Clox_VM_Interpret_Chunk(Clox_VM* const vm, Clox_Chunk* const chunk) {
    vm->chunk = chunk;
    vm->instruction_pointer = vm->chunk->code;
    vm->stack_top = vm->stack;


    for (;;) {
        Clox_Op_Code opcode = (Clox_Op_Code)vm->instruction_pointer[0];
        
        #ifdef CLOX_DEBUG_TRACE_STACK
        printf("[");
        for(Clox_Value* stack_ptr = vm->stack; stack_ptr < vm->stack_top; ++stack_ptr)
        {
            Clox_Value_Print(*stack_ptr);
            printf(" ");
        }
        printf("]\n");
        #endif // CLOX_DEBUG_TRACE_STACK

        #ifdef CLOX_DEBUG_TRACE_EXECUTION
        Clox_Chunk_Print_Op_Code(vm->chunk, (uint32_t)(vm->instruction_pointer - vm->chunk->code));
        #endif // CLOX_DEBUG_TRACE_EXECUTION


        switch (opcode) {
            // case OP_RETURN: {
            //     CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                
            //     vm->instruction_pointer += 1;
            //     return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_OK};
            // }break;
            case OP_CONSTANT: {
                uint8_t constant_idx = vm->instruction_pointer[1];
                Clox_Value constant_value = vm->chunk->constants.values[(size_t)constant_idx]; 
                Clox_VM_Stack_Push(vm, constant_value);
                vm->instruction_pointer += 2;
            }break;
            case OP_NIL: {
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NIL);
                vm->instruction_pointer += 1;
            } break;
            case OP_TRUE: {
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(true));
                vm->instruction_pointer += 1;
            } break;
            case OP_FALSE: {
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(false));
                vm->instruction_pointer += 1;
            } break;
            case OP_ARITHMETIC_NEGATION: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);

                double value = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(-value));
                vm->instruction_pointer += 1;
            }break;
            case OP_BOOLEAN_NEGATION: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                Clox_Value_Type top_type = Clox_VM_Stack_Peek(vm, 0).type; 

                if(top_type == CLOX_VALUE_TYPE_NIL) {
                    Clox_VM_Stack_Pop(vm); // pop the nil of the stack
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(true));
                    vm->instruction_pointer += 1;
                } else if (top_type == CLOX_VALUE_TYPE_BOOL) {
                    bool value = Clox_VM_Stack_Pop(vm).boolean;
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(!value));
                    vm->instruction_pointer += 1;
                } else {
                    // TODO(Al-Andrew, Diagnostic): diagnostic
                    return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_RUNTIME_ERROR};
                }
            }break;
            case OP_ADD: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);
                Clox_Value lhs = Clox_VM_Stack_Pop(vm);

                if(lhs.type != rhs.type) {
                    return (Clox_Interpret_Result){.status = INTERPRET_RUNTIME_ERROR};
                }

                if(CLOX_VALUE_IS_NUMBER(lhs) && CLOX_VALUE_IS_NUMBER(rhs)) {
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs.number + rhs.number));
                    vm->instruction_pointer += 1;
                }
                else if(CLOX_VALUE_IS_STRING(lhs), CLOX_VALUE_IS_STRING(rhs)) {
                    Clox_String* lhs_string = (Clox_String*)lhs.object;
                    Clox_String* rhs_string = (Clox_String*)rhs.object;

                    // TODO(Al-Andrew, GC): where does the memory for the old strings go?
                    // FIXME(Al-Andrwe): this is stupid
                    char* concat = malloc(lhs_string->length + rhs_string->length + 1);
                    unsigned int concat_length = lhs_string->length + rhs_string->length;
                    memcpy(concat, lhs_string->characters, lhs_string->length);
                    memcpy(concat + lhs_string->length, rhs_string->characters, rhs_string->length);
                    concat[rhs_string->length + lhs_string->length] = '\0';
                    Clox_String* concat_string = Clox_String_Create(vm, concat, concat_length);
                    free(concat);

                    Clox_VM_Stack_Push(vm, CLOX_VALUE_OBJECT(concat_string));
                    vm->instruction_pointer += 1;
                } else {
                    return (Clox_Interpret_Result){.status = INTERPRET_RUNTIME_ERROR};
                }
            }break;
            case OP_SUB: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double lhs = Clox_VM_Stack_Pop(vm).number;
                double rhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs - rhs));
                vm->instruction_pointer += 1;
            }break;
            case OP_MUL: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double lhs = Clox_VM_Stack_Pop(vm).number;
                double rhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs * rhs));
                vm->instruction_pointer += 1;
            }break;
            case OP_DIV: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double lhs = Clox_VM_Stack_Pop(vm).number;
                double rhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs / rhs));
                vm->instruction_pointer += 1;
            }break;
            case OP_EQUAL: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                Clox_Value lhs = Clox_VM_Stack_Pop(vm);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);

                if(lhs.type != rhs.type) {
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(false));
                    vm->instruction_pointer += 1;
                    break;
                }

                switch(lhs.type) {
                    case CLOX_VALUE_TYPE_NIL: {
                        Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(true));
                        vm->instruction_pointer += 1;
                    } break;
                    case CLOX_VALUE_TYPE_BOOL: {
                        Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs.boolean == rhs.boolean));
                        vm->instruction_pointer += 1;
                    } break;
                    case CLOX_VALUE_TYPE_NUMBER: {
                        Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs.number == rhs.number));
                        vm->instruction_pointer += 1;
                    } break;
                    case CLOX_VALUE_TYPE_OBJECT: {
                        
                        switch (lhs.object->type) {
                            case CLOX_OBJECT_TYPE_STRING: {
                                Clox_String* lhs_string = (Clox_String*)lhs.object;
                                Clox_String* rhs_string = (Clox_String*)rhs.object;

                                bool result = s8_compare((s8){.len = lhs_string->length, .string = lhs_string->characters}, (s8){.len = rhs_string->length, .string = rhs_string->characters}) == 0;
                                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(result));
                                vm->instruction_pointer += 1;
                            }break;
                        }

                    } break;
                }
            }break;
            case OP_GREATER: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double rhs = Clox_VM_Stack_Pop(vm).number;
                double lhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs > rhs));
                vm->instruction_pointer += 1;
            }break;
            case OP_LESS: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double rhs = Clox_VM_Stack_Pop(vm).number;
                double lhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs < rhs));
                vm->instruction_pointer += 1;
            }break;
            case OP_PRINT: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                Clox_Value value = Clox_VM_Stack_Pop(vm);
                Clox_Value_Print(value);
                printf("\n");
                vm->instruction_pointer += 1;
            } break;
            case OP_POP: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                Clox_Value value = Clox_VM_Stack_Pop(vm);
                (void)value;
                vm->instruction_pointer += 1;
            } break;
            case OP_DEFINE_GLOBAL: {
                uint8_t variable_name_index = vm->instruction_pointer[1];
                Clox_String* name = (Clox_String*)chunk->constants.values[variable_name_index].object;
                Clox_Value value = Clox_VM_Stack_Pop(vm);
                Clox_Hash_Table_Set(&vm->globals, name, value);
                vm->instruction_pointer += 2;
            } break;
            case OP_GET_GLOBAL: {
                uint8_t variable_name_index = vm->instruction_pointer[1];
                Clox_String* name = (Clox_String*)chunk->constants.values[variable_name_index].object;
                Clox_Value value = {0};
                if(!Clox_Hash_Table_Get(&vm->globals, name, &value)) {
                    return Clox_VM_Runtime_Error(vm, "Undefined variable '%s'.", name->characters);
                }
                Clox_VM_Stack_Push(vm, value);
                vm->instruction_pointer += 2;
            } break;
            case OP_SET_GLOBAL: {
                uint8_t variable_name_index = vm->instruction_pointer[1];
                Clox_String* name = (Clox_String*)chunk->constants.values[variable_name_index].object;
                if (Clox_Hash_Table_Set(&vm->globals, name, Clox_VM_Stack_Peek(vm, 0))) { // NOTE(Al-Andrew): we generate a pop instruction for the expression. thats why we only peek here
                    Clox_Hash_Table_Remove(&vm->globals, name); 
                    return Clox_VM_Runtime_Error(vm, "Undefined variable '%s'.", name->characters);
                }
                vm->instruction_pointer += 2;
            } break;
            case OP_GET_LOCAL: {
                uint8_t variable_index = vm->instruction_pointer[1];

                Clox_VM_Stack_Push(vm, vm->stack[variable_index]); 
                vm->instruction_pointer += 2;
            } break;
            case OP_SET_LOCAL: {
                uint8_t variable_index = vm->instruction_pointer[1];
                Clox_Value value = Clox_VM_Stack_Peek(vm, 0);
                vm->stack[variable_index] = value;
                vm->instruction_pointer += 2;
            } break;
            default: {

                return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_COMPILE_ERROR, .message = "Unknown instruction."};
            }break;
        }
    }

    CLOX_UNREACHABLE();
}

Clox_Interpret_Result Clox_VM_Interpret_Source(Clox_VM* vm, const char* source) {
    Clox_Chunk chunk = Clox_Chunk_New_Empty();
    Clox_Interpret_Result result = {0};

    do {

        if (!Clox_Compile_Source_To_Chunk(vm, source, &chunk)) {
            result.return_value = CLOX_VALUE_NIL;
            result.status = INTERPRET_COMPILE_ERROR;            
            break;
        }

        vm->chunk = &chunk;
        vm->instruction_pointer = vm->chunk->code;
        result = Clox_VM_Interpret_Chunk(vm, &chunk);
    } while(false);

    Clox_Chunk_Delete(&chunk);
    return result;
}