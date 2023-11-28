#include "common.h"
#include "chunk.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    Clox_VM vm = Clox_VM_New_Empty();
    Clox_Chunk chunk = Clox_Chunk_New_Empty();

    Clox_Chunk_Push(&chunk, OP_CONSTANT, 0);
    Clox_Chunk_Push(&chunk, Clox_Chunk_Push_Constant(&chunk, 1.2), 0);

    Clox_Chunk_Push(&chunk, OP_CONSTANT, 0);
    Clox_Chunk_Push(&chunk, Clox_Chunk_Push_Constant(&chunk, 3.4), 0);

    Clox_Chunk_Push(&chunk, OP_ADD, 0);

    Clox_Chunk_Push(&chunk, OP_CONSTANT, 0);
    Clox_Chunk_Push(&chunk, Clox_Chunk_Push_Constant(&chunk, 5.6), 0);
    
    Clox_Chunk_Push(&chunk, OP_DIV, 0);
    Clox_Chunk_Push(&chunk, OP_ARITHMETIC_NEGATION, 0);

    Clox_Chunk_Push(&chunk, OP_RETURN, 0);
    // Clox_Chunk_Print(&chunk, "My First Chunk");

    Clox_Interpret_Result result = Clox_VM_Interpret(&vm, &chunk);
    printf("[%s] Interpret done.\n", (result.status == INTERPRET_OK)?"SUCCESS":"FAILURE");
    printf("  status: %d\n", (uint32_t)result.status);
    printf("  return_value: ");
    Clox_Value_Print(result.return_value);
    printf("\n");
    Clox_Chunk_Delete(&chunk);
    Clox_VM_Delete(&vm);
    return 0;
}
