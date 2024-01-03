#include "compiler.h"
#include "scanner.h"
#include "chunk.h"

#define CLOX_DEBUG_PRINT_COMPILED_CHUNKS

typedef struct {
    Clox_Token current;
    Clox_Token previous;
    Clox_Scanner* scanner;
    bool had_error;
    bool panic_mode;
} Clox_Parser;

typedef enum {
    CLOX_PRECEDENCE_NONE,
    CLOX_PRECEDENCE_ASSIGNMENT,  // =
    CLOX_PRECEDENCE_OR,          // or
    CLOX_PRECEDENCE_AND,         // and
    CLOX_PRECEDENCE_EQUALITY,    // == !=
    CLOX_PRECEDENCE_COMPARISON,  // < > <= >=
    CLOX_PRECEDENCE_TERM,        // + -
    CLOX_PRECEDENCE_FACTOR,      // * /
    CLOX_PRECEDENCE_UNARY,       // ! -
    CLOX_PRECEDENCE_CALL,        // . ()
    CLOX_PRECEDENCE_PRIMARY
} Clox_Precedence;

typedef void (*Clox_Parse_Fn)(Clox_Parser*);

typedef struct {
  Clox_Parse_Fn prefix;
  Clox_Parse_Fn infix;
  Clox_Precedence precedence;
} Clox_Parse_Rule;

static void Clox_Compiler_Compile_Unary(Clox_Parser* parser);
static void Clox_Compiler_Compile_Binary(Clox_Parser* parser);
static void Clox_Compiler_Compile_Grouping(Clox_Parser* parser);
static void Clox_Compiler_Compile_Number(Clox_Parser* parser);

static Clox_Parse_Rule parse_rules[] = {
  [CLOX_TOKEN_LEFT_PAREN]    = {Clox_Compiler_Compile_Grouping, NULL, CLOX_PRECEDENCE_NONE},
  [CLOX_TOKEN_RIGHT_PAREN]   = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_LEFT_BRACE]    = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  }, 
  [CLOX_TOKEN_RIGHT_BRACE]   = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_COMMA]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_DOT]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_MINUS]         = {Clox_Compiler_Compile_Unary   , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_TERM  },
  [CLOX_TOKEN_PLUS]          = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_TERM  },
  [CLOX_TOKEN_SEMICOLON]     = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_SLASH]         = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_FACTOR},
  [CLOX_TOKEN_STAR]          = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_FACTOR},
  [CLOX_TOKEN_BANG]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_BANG_EQUAL]    = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_EQUAL]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_EQUAL_EQUAL]   = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_GREATER]       = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_GREATER_EQUAL] = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_LESS]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_LESS_EQUAL]    = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_IDENTIFIER]    = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_STRING]        = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_NUMBER]        = {Clox_Compiler_Compile_Number  , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_AND]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_CLASS]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_ELSE]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_FALSE]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_FOR]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_FUN]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_IF]            = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_NIL]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_OR]            = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_PRINT]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_RETURN]        = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_SUPER]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_THIS]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_TRUE]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_VAR]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_WHILE]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_ERROR]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_EOF]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
};

static void Clox_Compiler_Error_At_Token(Clox_Parser* parser, Clox_Token* token, const char* message) {
    if (parser->panic_mode) {
        return;
    }

    parser->panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == CLOX_TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == CLOX_TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->had_error = true;
}

static inline void Clox_Compiler_Error_At_Current(Clox_Parser* parser) {
    Clox_Compiler_Error_At_Token(parser, &parser->current, parser->current.start);
}

static inline void Clox_Compiler_Error_At_Previous(Clox_Parser* parser) {
    Clox_Compiler_Error_At_Token(parser, &parser->previous, parser->previous.start);
}

static inline void Clox_Compiler_Advance(Clox_Parser* parser) {

    parser->previous = parser->current;

    for (;;) {
        
        parser->current = Clox_Scanner_Get_Token(parser->scanner);
        
        if (parser->current.type != CLOX_TOKEN_ERROR) {
            break;
        }

        Clox_Compiler_Error_At_Current(parser);
    }
}

static inline void Clox_Compiler_Consume(Clox_Parser* parser, Clox_Token_Type tkn_type, char const* const message) {
    if (parser->current.type == tkn_type) {
        Clox_Compiler_Advance(parser);
        return;
    }

    Clox_Compiler_Error_At_Token(parser, &parser->current, message);
}

static Clox_Chunk* compiling_chunk;
static Clox_Chunk* Clox_Compiler_Current_Chunk() {
  return compiling_chunk;
}

static inline void Clox_Compiler_Emit_Byte(Clox_Parser* parser, uint8_t byte) {

    Clox_Chunk_Push(Clox_Compiler_Current_Chunk(), byte, parser->previous.line);
}

static inline void Clox_Compiler_Emit_Bytes(Clox_Parser* parser, uint32_t count, ...) {

    va_list args;
    va_start(args, count);
 
    for(unsigned int i = 0; i < count; ++i) {
        uint8_t byte = va_arg(args, int);
        Clox_Chunk_Push(Clox_Compiler_Current_Chunk(), byte, parser->previous.line);
    }
 
    va_end(args);
}

static inline uint8_t Clox_Compiler_Make_Constant(Clox_Parser* parser, Clox_Value value) {
  int constant = Clox_Chunk_Push_Constant(Clox_Compiler_Current_Chunk(), value);
  
  if (constant > UINT8_MAX) {
    Clox_Compiler_Error_At_Token(parser, &parser->previous, "Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static inline void Clox_Compiler_Emit_Constant(Clox_Parser* parser, Clox_Value value) {
    Clox_Compiler_Emit_Bytes(parser, 2, OP_CONSTANT, Clox_Compiler_Make_Constant(parser, value));
}

static inline void Clox_Compiler_Compile_Number(Clox_Parser* parser) {
    double value = strtod(parser->previous.start, NULL);
    Clox_Compiler_Emit_Constant(parser, CLOX_VALUE_NUMBER(value));
}

static inline Clox_Parse_Rule* Clox_Get_Parse_Rule(Clox_Token_Type operator);
static void Clox_Compiler_Parse_Precendence(Clox_Parser* parser, Clox_Precedence precedence) {
    Clox_Compiler_Advance(parser);
    Clox_Parse_Fn prefix_rule = Clox_Get_Parse_Rule(parser->previous.type)->prefix;
    
    if (prefix_rule == NULL) {
        Clox_Compiler_Error_At_Token(parser, &parser->previous, "Expect expression.");
        return;
    }

    prefix_rule(parser);

    while (precedence <= Clox_Get_Parse_Rule(parser->current.type)->precedence) {
        Clox_Compiler_Advance(parser);
        Clox_Parse_Fn infix_rule = Clox_Get_Parse_Rule(parser->previous.type)->infix;
        infix_rule(parser);
    }
}

static inline void Clox_Compiler_Compile_Expression(Clox_Parser* parser) {
    Clox_Compiler_Parse_Precendence(parser, CLOX_PRECEDENCE_ASSIGNMENT);
}

static void Clox_Compiler_Compile_Grouping(Clox_Parser* parser) {
    Clox_Compiler_Compile_Expression(parser);
    Clox_Compiler_Consume(parser, CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void Clox_Compiler_Compile_Unary(Clox_Parser* parser) {
    Clox_Token_Type operator = parser->previous.type;

    // Compile the operand.
    Clox_Compiler_Parse_Precendence(parser, CLOX_PRECEDENCE_UNARY);

    // Emit the operator instruction.
    switch (operator) {
        case CLOX_TOKEN_MINUS: {
            Clox_Compiler_Emit_Byte(parser, OP_ARITHMETIC_NEGATION); 
        } break;
        default: {
            CLOX_UNREACHABLE();
        } break;
    }
}

static inline Clox_Parse_Rule* Clox_Get_Parse_Rule(Clox_Token_Type operator) {
    return &parse_rules[operator];
}

static void Clox_Compiler_Compile_Binary(Clox_Parser* parser) {
  Clox_Token_Type operator = parser->previous.type;
  Clox_Parse_Rule* rule = Clox_Get_Parse_Rule(operator);
  Clox_Compiler_Parse_Precendence(parser, (Clox_Precedence)(rule->precedence + 1));

  switch (operator) {
    case CLOX_TOKEN_PLUS:          Clox_Compiler_Emit_Byte(parser, OP_ADD); break;
    case CLOX_TOKEN_MINUS:         Clox_Compiler_Emit_Byte(parser, OP_SUB); break;
    case CLOX_TOKEN_STAR:          Clox_Compiler_Emit_Byte(parser, OP_MUL); break;
    case CLOX_TOKEN_SLASH:         Clox_Compiler_Emit_Byte(parser, OP_DIV); break;
    default: CLOX_UNREACHABLE();
  }
}

static inline void Clox_Compiler_Emit_Return(Clox_Parser* parser) {
    Clox_Compiler_Emit_Byte(parser, OP_RETURN);
}

static inline void Clox_Compiler_End(Clox_Parser* parser) {
    Clox_Compiler_Emit_Return(parser);
}



bool Clox_Compile_Source_To_Chunk(const char* source, Clox_Chunk* chunk) {
    Clox_Parser parser = {0};
    Clox_Scanner scanner = Clox_Scanner_New(source);
    parser.scanner = &scanner;
    compiling_chunk = chunk;
    
    Clox_Compiler_Advance(&parser);
    Clox_Compiler_Compile_Expression(&parser);
    Clox_Compiler_Consume(&parser, CLOX_TOKEN_EOF, "Expected end of expression.");

    Clox_Compiler_End(&parser);

#ifdef CLOX_DEBUG_PRINT_COMPILED_CHUNKS
    if (!parser.had_error) {
        printf("Compiled chunk: \n");
        Clox_Chunk_Print(Clox_Compiler_Current_Chunk(), "code");
        printf("End of compiled chunk.\n");
        printf("==========\n");
    }
#endif
    return !parser.had_error;
}