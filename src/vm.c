#include "vm.h"
#include "common.h"
#include "compiler.h"

Clox_VM Clox_VM_New_Empty() {
    return (Clox_VM){0};
}

void Clox_VM_Delete(Clox_VM* const vm) {
    // NOTE(Al-Andrew, Leak): do we own the chunk?
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
            case OP_RETURN: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);
                
                vm->instruction_pointer += 1;
                return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_OK};
            }break;
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
                CLOX_VM_ASSURE_STACK_TYPE_0(CLOX_VALUE_TYPE_NUMBER);
                CLOX_VM_ASSURE_STACK_TYPE_1(CLOX_VALUE_TYPE_NUMBER);

                double lhs = Clox_VM_Stack_Pop(vm).number;
                double rhs = Clox_VM_Stack_Pop(vm).number;
                Clox_VM_Stack_Push(vm, CLOX_VALUE_NUMBER(lhs + rhs));
                vm->instruction_pointer += 1;
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
            default: {

                return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_COMPILE_ERROR};
            }break;
        }
    }

    CLOX_UNREACHABLE();
}

Clox_Interpret_Result Clox_VM_Interpret_Source(Clox_VM* vm, const char* source) {
    Clox_Chunk chunk = Clox_Chunk_New_Empty();
    Clox_Interpret_Result result = {0};

    do {

        if (!Clox_Compile_Source_To_Chunk(source, &chunk)) {
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