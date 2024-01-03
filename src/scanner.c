#include "scanner.h"


Clox_Scanner Clox_Scanner_New(const char* source) {
    return (Clox_Scanner){.current = source, .start = source, .line = 1};
}

static inline bool Clox_Scanner_Is_EOF(Clox_Scanner* scanner) {
    return *scanner->current == '\0';
}

static inline Clox_Token Clox_Scanner_Make_Token(Clox_Scanner* scanner, Clox_Token_Type type) {
    return (Clox_Token){
        .start = scanner->start,
        .length = (int)((unsigned long long)scanner->current - (unsigned long long)scanner->start),
        .type = type,
        .line = scanner->line
    };
}

static inline Clox_Token Clox_Scanner_Make_Error(Clox_Scanner* scanner, const char* message, int message_len) {
    return (Clox_Token){
        .start = message,
        .length = message_len,
        .type = CLOX_TOKEN_ERROR,
        .line = scanner->line
    };
}

static inline char Clox_Scanner_Peek(Clox_Scanner* scanner) {
    return *scanner->current;
}

static inline char Clox_Scanner_Peek_Next(Clox_Scanner* scanner) {
    if (Clox_Scanner_Is_EOF(scanner)) return '\0';
    return *(scanner->current+1);
}

static inline char Clox_Scanner_Advance(Clox_Scanner* scanner) {
    return *scanner->current++;
}

static inline void Clox_Scanner_Skip_Whitespace(Clox_Scanner* scanner) {
    for (;;) {
        char c = Clox_Scanner_Peek(scanner);
        switch (c) {
            case ' ':  // falltrough
            case '\r': // falltrough
            case '\t': {
                Clox_Scanner_Advance(scanner);
            } break;
            case '\n': {
                scanner->line++;
                Clox_Scanner_Advance(scanner);
            } break;
            case '/': {

                if (Clox_Scanner_Peek_Next(scanner) == '/') {
                    // A comment goes until the end of the line.
                    while (Clox_Scanner_Peek(scanner) != '\n' && !Clox_Scanner_Is_EOF(scanner)) {
                        Clox_Scanner_Advance(scanner);   
                    }
                } else {
                    return;
                }
            } break;
            default:
                return;
        }
    }
}


static inline bool Clox_Scanner_Advance_If_Matches(Clox_Scanner* scanner, char target) {
    if (Clox_Scanner_Is_EOF(scanner)) return false;
    char peek = Clox_Scanner_Peek(scanner);
    if(peek != target) {
        return false;
    }
    ++scanner->current;
    return true;
}

static inline Clox_Token Clox_Scanner_Make_String(Clox_Scanner* scanner) {
    while (Clox_Scanner_Peek(scanner) != '"' && !Clox_Scanner_Is_EOF(scanner)) {
    if (Clox_Scanner_Peek(scanner) == '\n') scanner->line++;
        Clox_Scanner_Advance(scanner);
    }

    if (Clox_Scanner_Is_EOF(scanner)) {
        return Clox_Scanner_Make_Error(scanner, ls8$("Unterminated string."));
    }
    // The closing quote.
    Clox_Scanner_Advance(scanner);
    return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_STRING);
}

static inline bool Is_Number_Char(char c) {
    return c >= '0' && c <= '9'; 
}

static inline Clox_Token Clox_Scanner_Make_Number(Clox_Scanner* scanner) {
    
    while (Is_Number_Char(Clox_Scanner_Peek(scanner))) {
        Clox_Scanner_Advance(scanner); 
    }

    // Look for a fractional part.
    if (Clox_Scanner_Peek(scanner) == '.' && Is_Number_Char(Clox_Scanner_Peek_Next(scanner))) {
        // Consume the ".".
        Clox_Scanner_Advance(scanner);

        while (Is_Number_Char(Clox_Scanner_Peek(scanner))) {
            Clox_Scanner_Advance(scanner);
        }
    }

    return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_NUMBER);
}

static inline bool Is_Identifier_Char(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static const s8 s_clox_keywords[] = {
    {ls8$("and")},
    {ls8$("class")},
    {ls8$("else")},
    {ls8$("false")},
    {ls8$("for")},
    {ls8$("fun")},
    {ls8$("if")},
    {ls8$("nil")},
    {ls8$("or")},
    {ls8$("print")},
    {ls8$("return")},
    {ls8$("super")},
    {ls8$("this")},
    {ls8$("true")},
    {ls8$("var")},
    {ls8$("while")},
};


#define static_array_count$(arr) sizeof(arr)/sizeof(arr[0])

static inline Clox_Token_Type Clox_Scanner_Get_Identifier_Type(Clox_Scanner* scanner) {
        
    const s8 ident = {.string = scanner->start, .len = (int)((unsigned long long)scanner->current - (unsigned long long)scanner->start)};

    for(unsigned int i = 0; i < static_array_count$(s_clox_keywords); ++i) {
        s8 potential = s_clox_keywords[i];

        if(s8_compare(potential, ident) == 0) {
            return CLOX_TOKEN_AND + i;
        } 
    }
    
    return CLOX_TOKEN_IDENTIFIER;
}

static inline Clox_Token Clox_Scanner_Make_Identifier(Clox_Scanner* scanner) {
    while (Is_Identifier_Char(Clox_Scanner_Peek(scanner)) || Is_Number_Char(Clox_Scanner_Peek(scanner))) {
        Clox_Scanner_Advance(scanner);
    }
    return Clox_Scanner_Make_Token(scanner, Clox_Scanner_Get_Identifier_Type(scanner));
}

Clox_Token Clox_Scanner_Get_Token(Clox_Scanner* scanner) {
    Clox_Scanner_Skip_Whitespace(scanner);
    scanner->start = scanner->current;

    if (Clox_Scanner_Is_EOF(scanner)) {
        return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_EOF);
    };

    char c = Clox_Scanner_Advance(scanner);

    if(Is_Number_Char(c)) {
        return Clox_Scanner_Make_Number(scanner);
    } else if (Is_Identifier_Char(c)) {
        return Clox_Scanner_Make_Identifier(scanner);
    }

    switch (c) {
        case '(': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_LEFT_PAREN);
        case ')': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_RIGHT_PAREN);
        case '{': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_LEFT_BRACE);
        case '}': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_RIGHT_BRACE);
        case ';': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_SEMICOLON);
        case ',': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_COMMA);
        case '.': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_DOT);
        case '-': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_MINUS);
        case '+': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_PLUS);
        case '/': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_SLASH);
        case '*': return Clox_Scanner_Make_Token(scanner, CLOX_TOKEN_STAR);
        case '!': return Clox_Scanner_Make_Token(scanner, (Clox_Scanner_Advance_If_Matches(scanner, '=') ? CLOX_TOKEN_BANG_EQUAL : CLOX_TOKEN_BANG));
        case '=': return Clox_Scanner_Make_Token(scanner, (Clox_Scanner_Advance_If_Matches(scanner, '=') ? CLOX_TOKEN_EQUAL_EQUAL : CLOX_TOKEN_EQUAL));
        case '<': return Clox_Scanner_Make_Token(scanner, (Clox_Scanner_Advance_If_Matches(scanner, '=') ? CLOX_TOKEN_LESS_EQUAL : CLOX_TOKEN_LESS));
        case '>': return Clox_Scanner_Make_Token(scanner, (Clox_Scanner_Advance_If_Matches(scanner, '=') ? CLOX_TOKEN_GREATER_EQUAL : CLOX_TOKEN_GREATER));
        case '"': return Clox_Scanner_Make_String(scanner);
        
    }



  return Clox_Scanner_Make_Error(scanner, ls8$("Unexpected character."));
}