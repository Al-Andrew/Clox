#include "compiler.h"
#include "scanner.h"

void Clox_Compile_Source(const char* source) {
    Clox_Scanner scanner = Clox_Scanner_New(source);
    (void)scanner;

    int line = -1;
    for (;;) {
        Clox_Token token = Clox_Scanner_Get_Token(&scanner);
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }
        
        printf("%2d '%.*s'\n", token.type, token.length, token.start); 

        if (token.type == CLOX_TOKEN_EOF) break;
    }


}