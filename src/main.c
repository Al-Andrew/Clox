#include "common.h"
#include "chunk.h"
#include "vm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int Clox_Print_Help() {

    printf("clox - interpeter for the lox programming language, written in C\n");
    printf("\nUsage: clox [file]\n");
    printf("WHERE:\n");
    printf("    [file] - one file containing lox source code for the interpreter to run.\n");

    return 1;
}

int Clox_Repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        Clox_Interpret(line);
    }

    return 0;
}

char* Clox_Read_File(const char* path_to_file) {
    FILE* file = NULL;
    if(fopen_s(&file, path_to_file, "rb") != 0) {
        printf("[Error] Could not get descriptor for file %s.\n", path_to_file);
        return NULL;
    }

    if(file == NULL) {
        printf("[Error] Could not get descriptor for file %s.\n", path_to_file);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1); // TODO(Al-Andrew, AllocFailure): custom allocator

    if(buffer == NULL) {
        return NULL;
    }


    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

int Clox_Run_File(const char* path_to_file) {

    char* source = Clox_Read_File(path_to_file);
    Clox_Interpret_Result result = Clox_Interpret(source);
    free(source);
    source = NULL;

    return result.status;
}


int main(int argc, char** argv)
{
    // TODO(Al-Andrew, Args): make/use a proper command line argumnets parser
    if(argc == 1) {
        return Clox_Repl();
    } else if (argc == 2) {
        return Clox_Run_File(argv[1]);
    } else {
        return Clox_Print_Help();
    }
    CLOX_UNREACHABLE();

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
