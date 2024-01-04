#include "vm.h"
#include "common.h"
#include "compiler.h"
#include <float.h>
#include <string.h>

Clox_VM Clox_VM_New_Empty() {
    return (Clox_VM){0};
}

void Clox_VM_Reset_Stack(Clox_VM* vm) {
    vm->stack_top = vm->stack;
    vm->call_frame_count = 0;
}

void Clox_VM_Delete(Clox_VM* const vm) {
    // NOTE(Al-Andrew, Leak): do we own the chunk?

    Clox_Hash_Table_Destory(&vm->strings);
    Clox_Hash_Table_Destory(&vm->globals);
    Clox_Object* it = vm->objects;
    while(it != NULL) {
        Clox_Object* next = it->next_object;
        Clox_Object_Deallocate(vm, it);
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

    for (int i = vm->call_frame_count - 1; i >= 0; i--) {
        Clox_Call_Frame* frame = &vm->frames[i];
        Clox_Function* function = frame->function;
        size_t instruction = frame->instruction_pointer - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.source_lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->characters);
        }
    }

    Clox_VM_Reset_Stack(vm);
    return (Clox_Interpret_Result){.status = INTERPRET_RUNTIME_ERROR};
}

static bool Clox_VM_Call(Clox_VM* vm, Clox_Function* callee, int argCount) {

    if (argCount != callee->arity) {
        Clox_VM_Runtime_Error(vm, "Expected %d arguments but got %d.", callee->arity, argCount);
        return false;
    }
    if (vm->call_frame_count == CLOX_MAX_CALL_FRAMES) {
        Clox_VM_Runtime_Error(vm, "Stack overflow.");
        return false;
    }
    Clox_Call_Frame* frame = &vm->frames[vm->call_frame_count++];
    frame->function = callee;
    frame->instruction_pointer = callee->chunk.code;
    frame->slots = vm->stack_top - argCount - 1;
    return true;
}

static bool Clox_VM_Call_Value(Clox_VM* vm, Clox_Value callee, int argCount) {
  if (CLOX_VALUE_IS_OBJECT(callee)) {
    switch (callee.object->type) {
      case CLOX_OBJECT_TYPE_FUNCTION: 
        return Clox_VM_Call(vm, (Clox_Function*)callee.object, argCount);
      default:
        break; // Non-callable object type.
    }
  }
  Clox_VM_Runtime_Error(vm, "Can only call functions and classes.");
  return false;
}

Clox_Interpret_Result Clox_VM_Interpret_Function(Clox_VM* const vm, Clox_Function* function) {
    // vm->chunk = chunk;

    Clox_Call_Frame* frame = &vm->frames[vm->call_frame_count - 1];
    #define READ_BYTE() (*frame->instruction_pointer++)

    #define READ_SHORT() \
        (frame->instruction_pointer += 2, \
        (uint16_t)((frame->instruction_pointer[-2] << 8) | frame->instruction_pointer[-1]))

    #define READ_CONSTANT() \
        (frame->function->chunk.constants.values[READ_BYTE()])

    #define READ_STRING() ((Clox_String*)READ_CONSTANT().object)

    for (;;) {
        
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
        Clox_Chunk_Print_Op_Code(&frame->function->chunk, (uint32_t)(frame->instruction_pointer - frame->function->chunk.code));
        #endif // CLOX_DEBUG_TRACE_EXECUTION

        Clox_Op_Code opcode = (Clox_Op_Code)READ_BYTE();

        switch (opcode) {
            case OP_RETURN: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                
                Clox_Value result = Clox_VM_Stack_Pop(vm);
                vm->call_frame_count--;
                if (vm->call_frame_count == 0) {
                    Clox_VM_Stack_Pop(vm); //this pops the <script> function off the stack
                    return (Clox_Interpret_Result){.status = INTERPRET_OK, .return_value = result};
                }

                vm->stack_top = frame->slots;
                Clox_VM_Stack_Push(vm, result);
                frame = &vm->frames[vm->call_frame_count - 1];
            }break;
            case OP_CONSTANT: {
                Clox_Value constant_value = READ_CONSTANT();
                Clox_VM_Stack_Push(vm, constant_value);
            }break;
            case OP_NIL: {
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NIL);
            } break;
            case OP_TRUE: {
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(true));
            } break;
            case OP_FALSE: {
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(false));
            } break;
            case OP_ARITHMETIC_NEGATION: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);

                double value = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(-value));
            }break;
            case OP_BOOLEAN_NEGATION: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                Clox_Value_Type top_type = Clox_VM_Stack_Peek(vm, 0).type; 

                if(top_type == CLOX_VALUE_TYPE_NIL) {
                    Clox_VM_Stack_Pop(vm); // pop the nil of the stack
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(true));
                } else if (top_type == CLOX_VALUE_TYPE_BOOL) {
                    bool value = Clox_VM_Stack_Pop(vm).boolean;
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(!value));
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
            }break;
            case OP_MUL: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double lhs = Clox_VM_Stack_Pop(vm).number;
                double rhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs * rhs));
            }break;
            case OP_DIV: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double lhs = Clox_VM_Stack_Pop(vm).number;
                double rhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs / rhs));
            }break;
            case OP_EQUAL: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                Clox_Value lhs = Clox_VM_Stack_Pop(vm);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);

                if(lhs.type != rhs.type) {
                    Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(false));
                    break;
                }

                switch(lhs.type) {
                    case CLOX_VALUE_TYPE_NIL: {
                        Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(true));
                    } break;
                    case CLOX_VALUE_TYPE_BOOL: {
                        Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs.boolean == rhs.boolean));
                    } break;
                    case CLOX_VALUE_TYPE_NUMBER: {
                        Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs.number == rhs.number));
                    } break;
                    case CLOX_VALUE_TYPE_OBJECT: {
                        
                        switch (lhs.object->type) {
                            case CLOX_OBJECT_TYPE_STRING: {
                                Clox_String* lhs_string = (Clox_String*)lhs.object;
                                Clox_String* rhs_string = (Clox_String*)rhs.object;

                                bool result = s8_compare((s8){.len = lhs_string->length, .string = lhs_string->characters}, (s8){.len = rhs_string->length, .string = rhs_string->characters}) == 0;
                                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(result));
                            }break;
                            case CLOX_OBJECT_TYPE_FUNCTION: {
                                CLOX_UNREACHABLE(); // TODO(Al-Andrew): 
                            } break;
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
            }break;
            case OP_LESS: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);


                double rhs = Clox_VM_Stack_Pop(vm).number;
                double lhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_BOOL(lhs < rhs));
            }break;
            case OP_PRINT: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                Clox_Value value = Clox_VM_Stack_Pop(vm);
                Clox_Value_Print(value);
                printf("\n");
            } break;
            case OP_POP: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                Clox_Value value = Clox_VM_Stack_Pop(vm);
                (void)value;
            } break;
            case OP_DEFINE_GLOBAL: {
                Clox_String* name = READ_STRING();
                Clox_Value value = Clox_VM_Stack_Pop(vm);
                Clox_Hash_Table_Set(&vm->globals, name, value);
            } break;
            case OP_GET_GLOBAL: {
                Clox_String* name = READ_STRING();
                Clox_Value value = {0};
                if(!Clox_Hash_Table_Get(&vm->globals, name, &value)) {
                    return Clox_VM_Runtime_Error(vm, "Undefined variable '%s'.", name->characters);
                }
                Clox_VM_Stack_Push(vm, value);
            } break;
            case OP_SET_GLOBAL: {
                Clox_String* name = READ_STRING();
                if (Clox_Hash_Table_Set(&vm->globals, name, Clox_VM_Stack_Peek(vm, 0))) { // NOTE(Al-Andrew): we generate a pop instruction for the expression. thats why we only peek here
                    Clox_Hash_Table_Remove(&vm->globals, name); 
                    return Clox_VM_Runtime_Error(vm, "Undefined variable '%s'.", name->characters);
                }
            } break;
            case OP_GET_LOCAL: {
                uint8_t variable_index = READ_BYTE();

                Clox_VM_Stack_Push(vm, frame->slots[variable_index] ); 
            } break;
            case OP_SET_LOCAL: {
                uint8_t variable_index = READ_BYTE();
                Clox_Value value = Clox_VM_Stack_Peek(vm, 0);
                frame->slots[variable_index] = value;
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->instruction_pointer += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                Clox_Value condition = Clox_VM_Stack_Peek(vm, 0);
                if (Clox_Value_Is_Falsy(condition)) {
                    frame->instruction_pointer += offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->instruction_pointer -= offset;
            } break;
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!Clox_VM_Call_Value(vm, Clox_VM_Stack_Peek(vm, argCount), argCount)) {
                    return Clox_VM_Runtime_Error(vm, "Error while trying to call.");
                }
                frame = &vm->frames[vm->call_frame_count - 1];
            } break;
            default: {

                return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_COMPILE_ERROR, .message = "Unknown instruction."};
            }break;
        } // end switch
    } // end for

    CLOX_UNREACHABLE();
}

Clox_Interpret_Result Clox_VM_Interpret_Source(Clox_VM* vm, const char* source) {
    Clox_Interpret_Result result = {0};
    Clox_VM_Reset_Stack(vm);

    do {
        Clox_Function* top_level_function = Clox_Compile_Source_To_Function(vm, source); 
        if (top_level_function == NULL) {
            result.return_value = CLOX_VALUE_NIL;
            result.status = INTERPRET_COMPILE_ERROR;            
            break;
        }

        Clox_VM_Stack_Push(vm, CLOX_VALUE_OBJECT(top_level_function));
        Clox_VM_Call(vm, top_level_function, 0);

        result = Clox_VM_Interpret_Function(vm, top_level_function);
    } while(false);

    return result;
}