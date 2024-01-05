#include "common.h"
#include "chunk.h"
#include "vm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "memory.h"

int Clox_Print_Help() {

    printf("clox - interpeter for the lox programming language, written in C\n");
    printf("\nUsage: clox [file]\n");
    printf("WHERE:\n");
    printf("    [file] - one file containing lox source code for the interpreter to run.\n");

    return 1;
}


int Clox_Repl() {
    Clox_VM vm = {0};
    Clox_VM_Init(&vm);
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        if(s8_compare((s8){.len = (int)strlen(line) - 1, .string = line}, s8$("exit")) == 0) {
            break;
        }

        Clox_VM_Interpret_Source(&vm, line);
    }
    Clox_VM_Delete(&vm);
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

    char* buffer = (char*)malloc(fileSize + 1);

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
    Clox_VM vm = {0};
    Clox_VM_Init(&vm);

    Clox_Interpret_Result result = Clox_VM_Interpret_Source(&vm, source);
    free(source);
    source = NULL;
    Clox_VM_Delete(&vm);
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

    return 0;
}
