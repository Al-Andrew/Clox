#include "vm.h"
#include "common.h"


Clox_VM Clox_VM_New_Empty() {
    return (Clox_VM){0};
}

void Clox_VM_Delete(Clox_VM* const vm) {
    // NOTE(Al-Andrew, Leak): do we own the chunk?
}

static void Clox_VM_Stack_Push(Clox_VM* const vm, Clox_Value const value) {
    *(vm->stack_top++) = value;
}

static Clox_Value Clox_VM_Stack_Pop(Clox_VM* const vm) {
    return *(--vm->stack_top);
}

// NOTE(Al-Andrew): assumes `Clox_VM* const vm` is in scope and we're returning Clox_Interpret_Result
#define CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(N) { if((vm->stack_top - vm->stack) < N) return (Clox_Interpret_Result){.status = INTERPRET_COMPILE_ERROR}; }

Clox_Interpret_Result Clox_VM_Interpret(Clox_VM* const vm, Clox_Chunk* const chunk) {
    vm->chunk = chunk;
    vm->instruction_pointer = vm->chunk->code;
    vm->stack_top = vm->stack;


    for (;;) {
        Clox_Op_Code opcode = (Clox_Op_Code)vm->instruction_pointer[0];
        
        #ifdef CLOX_DEBUG_TRACE_EXECUTION
        Clox_Chunk_Print_Op_Code(vm->chunk, (uint32_t)(vm->instruction_pointer - vm->chunk->code));
        #endif // CLOX_DEBUG_TRACE_EXECUTION

        #ifdef CLOX_DEBUG_TRACE_STACK
        printf("[");
        for(Clox_Value* stack_ptr = vm->stack; stack_ptr < vm->stack_top; ++stack_ptr)
        {
            Clox_Value_Print(*stack_ptr);
            printf(" ");
        }
        printf("]\n");
        #endif // CLOX_DEBUG_TRACE_STACK

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
            case OP_ARITHMETIC_NEGATION: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(1);

                Clox_Value value = Clox_VM_Stack_Pop(vm);
                Clox_VM_Stack_Push(vm, -value);
                vm->instruction_pointer += 1;
            }break;
            case OP_ADD: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);

                Clox_Value lhs = Clox_VM_Stack_Pop(vm);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);
                Clox_VM_Stack_Push(vm, lhs + rhs);
                vm->instruction_pointer += 1;
            }break;
            case OP_SUB: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);

                Clox_Value lhs = Clox_VM_Stack_Pop(vm);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);
                Clox_VM_Stack_Push(vm, lhs - rhs);
                vm->instruction_pointer += 1;
            }break;
            case OP_MUL: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);

                Clox_Value lhs = Clox_VM_Stack_Pop(vm);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);
                Clox_VM_Stack_Push(vm, lhs * rhs);
                vm->instruction_pointer += 1;
            }break;
            case OP_DIV: {
                CLOX_VM_ASSURE_STACK_CONTAINS_AT_LEAST(2);

                Clox_Value lhs = Clox_VM_Stack_Pop(vm);
                Clox_Value rhs = Clox_VM_Stack_Pop(vm);
                Clox_VM_Stack_Push(vm, lhs / rhs);
                vm->instruction_pointer += 1;
            }break;
            default: {

                return (Clox_Interpret_Result){.return_value = Clox_VM_Stack_Pop(vm), .status = INTERPRET_COMPILE_ERROR};
            }break;
        }
    }

    CLOX_UNREACHABLE();
}