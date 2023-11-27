#include "common.h"
#include "chunk.h"

int main(int argc, char** argv)
{
    Clox_Chunk chunk = Clox_Chunk_New_Empty();

    Clox_Chunk_Push(&chunk, OP_RETURN);
    Clox_Chunk_Push(&chunk, OP_CONSTANT);
    Clox_Chunk_Push(&chunk, Clox_Chunk_Push_Constant(&chunk, 420.0));

    Clox_Chunk_Print(&chunk, "My First Chunk");

    Clox_Chunk_Delete(&chunk);
    return 0;
}
