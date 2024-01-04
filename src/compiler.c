#include "compiler.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"
#include <stdint.h>
#include <string.h>

#define CLOX_DEBUG_PRINT_COMPILED_CHUNKS


typedef struct {
    Clox_Token name;
    int depth;
} Clox_Local;

typedef enum {
  CLOX_FUNCTION_TYPE_FUNCTION,
  CLOX_FUNCTION_TYPE_SCRIPT
} Clox_Function_Type;

typedef struct {
    Clox_Function* function;
    Clox_Function_Type type;
    Clox_Local locals[UINT8_MAX + 1];
    int localCount;
    int scopeDepth;
} Clox_Compiler;

typedef struct {
    Clox_Token current;
    Clox_Token previous;
    Clox_Scanner* scanner;
    Clox_VM* vm;
    Clox_Compiler* compiler;
    bool had_error;
    bool panic_mode;
} Clox_Parser;

static void Clox_Compiler_Init(Clox_Parser* parser, Clox_Compiler* compiler, Clox_Function_Type type) {
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    memset(compiler->locals, 0, sizeof(compiler->locals));
    compiler->function = Clox_Function_Create_Empty(parser->vm);

    Clox_Local* local = &parser->compiler->locals[parser->compiler->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

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

typedef void (*Clox_Parse_Fn)(Clox_Parser*, bool);

typedef struct {
  Clox_Parse_Fn prefix;
  Clox_Parse_Fn infix;
  Clox_Precedence precedence;
} Clox_Parse_Rule;

static void Clox_Compiler_Compile_Unary(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_Binary(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_Grouping(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_Number(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_String(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_Literal(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_Variable(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_And_(Clox_Parser* parser, bool);
static void Clox_Compiler_Compile_Or_(Clox_Parser* parser, bool);

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
  [CLOX_TOKEN_BANG]          = {Clox_Compiler_Compile_Unary   , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_BANG_EQUAL]    = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_EQUALITY  },
  [CLOX_TOKEN_EQUAL]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_EQUAL_EQUAL]   = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_EQUALITY  },
  [CLOX_TOKEN_GREATER]       = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_COMPARISON  },
  [CLOX_TOKEN_GREATER_EQUAL] = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_COMPARISON  },
  [CLOX_TOKEN_LESS]          = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_COMPARISON  },
  [CLOX_TOKEN_LESS_EQUAL]    = {NULL                          , Clox_Compiler_Compile_Binary, CLOX_PRECEDENCE_COMPARISON  },
  [CLOX_TOKEN_IDENTIFIER]    = {Clox_Compiler_Compile_Variable, NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_STRING]        = {Clox_Compiler_Compile_String  , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_NUMBER]        = {Clox_Compiler_Compile_Number  , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_AND]           = {Clox_Compiler_Compile_And_    , NULL                        , CLOX_PRECEDENCE_AND  },
  [CLOX_TOKEN_CLASS]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_ELSE]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_FALSE]         = {Clox_Compiler_Compile_Literal , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_FOR]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_FUN]           = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_IF]            = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_NIL]           = {Clox_Compiler_Compile_Literal , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_OR]            = {Clox_Compiler_Compile_Or_     , NULL                        , CLOX_PRECEDENCE_OR  },
  [CLOX_TOKEN_PRINT]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_RETURN]        = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_SUPER]         = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_THIS]          = {NULL                          , NULL                        , CLOX_PRECEDENCE_NONE  },
  [CLOX_TOKEN_TRUE]          = {Clox_Compiler_Compile_Literal , NULL                        , CLOX_PRECEDENCE_NONE  },
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

// static inline void Clox_Compiler_Error_At_Previous(Clox_Parser* parser) {
//     Clox_Compiler_Error_At_Token(parser, &parser->previous, parser->previous.start);
// }

static inline void Clox_Compiler_Error(Clox_Parser* parser, const char* message) {
    Clox_Compiler_Error_At_Token(parser, &parser->previous, message);
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

static inline bool Clox_Compiler_Match(Clox_Parser* parser, Clox_Token_Type type) {
    if (parser->current.type != type) {
        return false;
    }
    Clox_Compiler_Advance(parser);
    return true;
}

// static Clox_Chunk* compiling_chunk;
static Clox_Chunk* Clox_Compiler_Current_Chunk(Clox_Parser* parser) {
  return &parser->compiler->function->chunk;
}

static inline void Clox_Compiler_Emit_Byte(Clox_Parser* parser, uint8_t byte) {

    Clox_Chunk_Push(Clox_Compiler_Current_Chunk(parser), byte, parser->previous.line);
}

static inline void Clox_Compiler_Emit_Bytes(Clox_Parser* parser, uint32_t count, ...) {

    va_list args;
    va_start(args, count);
 
    for(unsigned int i = 0; i < count; ++i) {
        uint8_t byte = va_arg(args, int);
        Clox_Chunk_Push(Clox_Compiler_Current_Chunk(parser), byte, parser->previous.line);
    }
 
    va_end(args);
}

static inline uint8_t Clox_Compiler_Make_Constant(Clox_Parser* parser, Clox_Value value) {
  int constant = Clox_Chunk_Push_Constant(Clox_Compiler_Current_Chunk(parser), value);
  
  if (constant > UINT8_MAX) {
    Clox_Compiler_Error_At_Token(parser, &parser->previous, "Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static inline void Clox_Compiler_Emit_Constant(Clox_Parser* parser, Clox_Value value) {
    Clox_Compiler_Emit_Bytes(parser, 2, OP_CONSTANT, Clox_Compiler_Make_Constant(parser, value));
}

static inline void Clox_Compiler_Compile_Number(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
    double value = strtod(parser->previous.start, NULL);
    Clox_Compiler_Emit_Constant(parser, CLOX_VALUE_NUMBER(value));
}

static inline void Clox_Compiler_Compile_String(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
    Clox_Compiler_Emit_Constant(parser, CLOX_VALUE_OBJECT(Clox_String_Create(parser->vm, parser->previous.start + 1, parser->previous.length - 2)));
}

static inline void Clox_Compiler_Compile_Literal(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
    switch (parser->previous.type) {
        case CLOX_TOKEN_FALSE: Clox_Compiler_Emit_Byte(parser, OP_FALSE); break;
        case CLOX_TOKEN_NIL: Clox_Compiler_Emit_Byte(parser, OP_NIL); break;
        case CLOX_TOKEN_TRUE: Clox_Compiler_Emit_Byte(parser, OP_TRUE); break;
        default: CLOX_UNREACHABLE();
  }
}

static inline Clox_Parse_Rule* Clox_Get_Parse_Rule(Clox_Token_Type operator);
static void Clox_Compiler_Parse_Precendence(Clox_Parser* parser, Clox_Precedence precedence) {
    Clox_Compiler_Advance(parser);
    Clox_Parse_Fn prefix_rule = Clox_Get_Parse_Rule(parser->previous.type)->prefix;
    
    if (prefix_rule == NULL) {
        Clox_Compiler_Error_At_Token(parser, &parser->previous, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= CLOX_PRECEDENCE_ASSIGNMENT;
    prefix_rule(parser, canAssign);

    while (precedence <= Clox_Get_Parse_Rule(parser->current.type)->precedence) {
        Clox_Compiler_Advance(parser);
        Clox_Parse_Fn infix_rule = Clox_Get_Parse_Rule(parser->previous.type)->infix;
        infix_rule(parser, canAssign);
    }

    if (canAssign && Clox_Compiler_Match(parser, CLOX_TOKEN_EQUAL)) {
        Clox_Compiler_Error_At_Token(parser, &parser->previous, "Invalid assignment target.");
    }
}

static inline void Clox_Compiler_Compile_Expression(Clox_Parser* parser) {
    Clox_Compiler_Parse_Precendence(parser, CLOX_PRECEDENCE_ASSIGNMENT);
}

static void Clox_Compiler_Compile_Grouping(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
    Clox_Compiler_Compile_Expression(parser);
    Clox_Compiler_Consume(parser, CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void Clox_Compiler_Compile_Unary(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
    Clox_Token_Type operator = parser->previous.type;

    // Compile the operand.
    Clox_Compiler_Parse_Precendence(parser, CLOX_PRECEDENCE_UNARY);

    // Emit the operator instruction.
    switch (operator) {
        case CLOX_TOKEN_MINUS: {
            Clox_Compiler_Emit_Byte(parser, OP_ARITHMETIC_NEGATION); 
        } break;
        case CLOX_TOKEN_BANG: {
            Clox_Compiler_Emit_Byte(parser, OP_BOOLEAN_NEGATION); 
        } break;
        default: {
            CLOX_UNREACHABLE();
        } break;
    }
}

static inline Clox_Parse_Rule* Clox_Get_Parse_Rule(Clox_Token_Type operator) {
    return &parse_rules[operator];
}

static void Clox_Compiler_Compile_Binary(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
  Clox_Token_Type operator = parser->previous.type;
  Clox_Parse_Rule* rule = Clox_Get_Parse_Rule(operator);
  Clox_Compiler_Parse_Precendence(parser, (Clox_Precedence)(rule->precedence + 1));

  switch (operator) {
    case CLOX_TOKEN_PLUS:          Clox_Compiler_Emit_Byte(parser, OP_ADD); break;
    case CLOX_TOKEN_MINUS:         Clox_Compiler_Emit_Byte(parser, OP_SUB); break;
    case CLOX_TOKEN_STAR:          Clox_Compiler_Emit_Byte(parser, OP_MUL); break;
    case CLOX_TOKEN_SLASH:         Clox_Compiler_Emit_Byte(parser, OP_DIV); break;
    case CLOX_TOKEN_EQUAL_EQUAL:   Clox_Compiler_Emit_Byte(parser, OP_EQUAL); break;
    case CLOX_TOKEN_BANG_EQUAL:    Clox_Compiler_Emit_Bytes(parser, 2, OP_EQUAL, OP_BOOLEAN_NEGATION); break;
    case CLOX_TOKEN_GREATER_EQUAL: Clox_Compiler_Emit_Bytes(parser, 2, OP_LESS, OP_BOOLEAN_NEGATION); break;
    case CLOX_TOKEN_GREATER:       Clox_Compiler_Emit_Bytes(parser, 1, OP_GREATER); break;
    case CLOX_TOKEN_LESS_EQUAL:    Clox_Compiler_Emit_Bytes(parser, 2, OP_GREATER, OP_BOOLEAN_NEGATION); break;
    case CLOX_TOKEN_LESS:          Clox_Compiler_Emit_Bytes(parser, 1, OP_LESS); break;
    default: CLOX_UNREACHABLE();
  }
}

static void Clox_Compiler_Compile_Print_Statement(Clox_Parser* parser) {
    Clox_Compiler_Compile_Expression(parser);
    Clox_Compiler_Consume(parser, CLOX_TOKEN_SEMICOLON, "Expect ';' after value.");
    Clox_Compiler_Emit_Byte(parser, OP_PRINT);
}

static void Clox_Compiler_Compile_Expression_Statement(Clox_Parser* parser) {
    Clox_Compiler_Compile_Expression(parser);
    Clox_Compiler_Consume(parser, CLOX_TOKEN_SEMICOLON, "Expect ';' after value.");
    Clox_Compiler_Emit_Byte(parser, OP_POP);
}

static bool Clox_Compiler_Check(Clox_Parser* parser, Clox_Token_Type type) {
    return parser->current.type == type;
}

static void Clox_Compiler_Begin_Scope(Clox_Parser* parser) {
    parser->compiler->scopeDepth++;
}


void Clox_Compiler_Compile_Declaration(Clox_Parser* parser);

static void Clox_Compiler_Compile_Block(Clox_Parser*  parser) {
    while (!Clox_Compiler_Check(parser, CLOX_TOKEN_RIGHT_BRACE) && !Clox_Compiler_Check(parser, CLOX_TOKEN_EOF)) {
        Clox_Compiler_Compile_Declaration(parser);
    }

    Clox_Compiler_Consume(parser, CLOX_TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void Clox_Compiler_End_Scope(Clox_Parser* parser) {
    parser->compiler->scopeDepth--;

    bool have_what_to_pop = parser->compiler->localCount > 0;
    bool should_pop = parser->compiler->locals[parser->compiler->localCount - 1].depth > parser->compiler->scopeDepth;
    while (have_what_to_pop && should_pop) {
        Clox_Compiler_Emit_Byte(parser, OP_POP);
        parser->compiler->localCount--;
        have_what_to_pop = parser->compiler->localCount > 0;
        should_pop = parser->compiler->locals[parser->compiler->localCount - 1].depth > parser->compiler->scopeDepth;    
    }
}

static void Clox_Compiler_Compile_Statement(Clox_Parser* parser);

static int Clox_Compiler_Emit_Jump(Clox_Parser* parser, uint8_t instruction) {
    Clox_Compiler_Emit_Bytes(parser, 3, instruction, 0xff, 0xff);
    return Clox_Compiler_Current_Chunk(parser)->used - 2;
}

static void Clox_Compiler_Patch_Jump(Clox_Parser* parser, int offset) {
    // NOTE(Al-Andrew): -2 to adjust for the bytecode for the jump offset itself.
    int jump = Clox_Compiler_Current_Chunk(parser)->used - offset - 2;

    if (jump > UINT16_MAX) {
        Clox_Compiler_Error(parser, "Too much code to jump over.");
    }

    Clox_Compiler_Current_Chunk(parser)->code[offset] = (jump >> 8) & 0xff;
    Clox_Compiler_Current_Chunk(parser)->code[offset + 1] = jump & 0xff;
}

static void Clox_Compiler_Emit_Loop(Clox_Parser* parser, int loop_start) {
    Clox_Compiler_Emit_Byte(parser, OP_LOOP);

    int offset = Clox_Compiler_Current_Chunk(parser)->used - loop_start + 2;
    if (offset > UINT16_MAX) {
        Clox_Compiler_Error(parser, "Loop body too large.");
    }

    Clox_Compiler_Emit_Bytes(parser, 2, (offset >> 8) & 0xff, offset & 0xff);
}

static void Clox_Compiler_Compile_If_Statement(Clox_Parser* parser) {
    Clox_Compiler_Consume(parser, CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    Clox_Compiler_Compile_Expression(parser);
    Clox_Compiler_Consume(parser, CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

    int thenJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP_IF_FALSE);
    Clox_Compiler_Emit_Byte(parser, OP_POP);
    Clox_Compiler_Compile_Statement(parser);
    int elseJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP);

    Clox_Compiler_Patch_Jump(parser, thenJump);
    Clox_Compiler_Emit_Byte(parser, OP_POP);

    if (Clox_Compiler_Match(parser, CLOX_TOKEN_ELSE)) {
        Clox_Compiler_Compile_Statement(parser);
    } 
    Clox_Compiler_Patch_Jump(parser, elseJump);
}

static void Clox_Copmiler_Compile_While_Statement(Clox_Parser* parser) {
    int loopStart = Clox_Compiler_Current_Chunk(parser)->used;
    Clox_Compiler_Consume(parser, CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    Clox_Compiler_Compile_Expression(parser);
    Clox_Compiler_Consume(parser, CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

    int exitJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP_IF_FALSE);
    Clox_Compiler_Emit_Byte(parser, OP_POP);
    Clox_Compiler_Compile_Statement(parser); // NOTE(Al-Andrew): the loop body
    Clox_Compiler_Emit_Loop(parser, loopStart);

    Clox_Compiler_Patch_Jump(parser, exitJump);
    Clox_Compiler_Emit_Byte(parser, OP_POP);
}

static void Clox_Compiler_Compile_Variable_Declaration(Clox_Parser* parser);

static void Clox_Copmiler_Compile_For_Statement(Clox_Parser* parser) {
    Clox_Compiler_Begin_Scope(parser);
    
    Clox_Compiler_Consume(parser, CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    if (Clox_Compiler_Match(parser, CLOX_TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (Clox_Compiler_Match(parser, CLOX_TOKEN_VAR)) {
        Clox_Compiler_Compile_Variable_Declaration(parser);
    } else {
        Clox_Compiler_Compile_Expression_Statement(parser);
    }
    int loopStart = Clox_Compiler_Current_Chunk(parser)->used;

    int exitJump = -1;
    if (!Clox_Compiler_Match(parser,CLOX_TOKEN_SEMICOLON)) {
        Clox_Compiler_Compile_Expression(parser);
        Clox_Compiler_Consume(parser, CLOX_TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP_IF_FALSE);
        Clox_Compiler_Emit_Byte(parser, OP_POP); // Condition.
    }
    if (!Clox_Compiler_Match(parser, CLOX_TOKEN_RIGHT_PAREN)) {
        int bodyJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP);
        int incrementStart = Clox_Compiler_Current_Chunk(parser)->used;
        Clox_Compiler_Compile_Expression(parser);
        Clox_Compiler_Emit_Byte(parser, OP_POP);
        Clox_Compiler_Consume(parser, CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        Clox_Compiler_Emit_Loop(parser, loopStart);
        loopStart = incrementStart;
        Clox_Compiler_Patch_Jump(parser, bodyJump);
    }
    
    Clox_Compiler_Compile_Statement(parser);
    Clox_Compiler_Emit_Loop(parser, loopStart);

    if (exitJump != -1) {
        Clox_Compiler_Patch_Jump(parser, exitJump);
        Clox_Compiler_Emit_Byte(parser, OP_POP); // Condition.
    }

    Clox_Compiler_End_Scope(parser);
}

static void Clox_Compiler_Compile_Statement(Clox_Parser* parser) {
    if (Clox_Compiler_Match(parser, CLOX_TOKEN_PRINT)) {
        Clox_Compiler_Compile_Print_Statement(parser);
    } else if (Clox_Compiler_Match(parser, CLOX_TOKEN_LEFT_BRACE)) {
        Clox_Compiler_Begin_Scope(parser);
        Clox_Compiler_Compile_Block(parser);
        Clox_Compiler_End_Scope(parser);
    } else if (Clox_Compiler_Match(parser, CLOX_TOKEN_IF)) {
        Clox_Compiler_Compile_If_Statement(parser);
    } else if (Clox_Compiler_Match(parser, CLOX_TOKEN_WHILE)) {
        Clox_Copmiler_Compile_While_Statement(parser);
    } else if (Clox_Compiler_Match(parser, CLOX_TOKEN_FOR)) {
        Clox_Copmiler_Compile_For_Statement(parser);
    } else {
        Clox_Compiler_Compile_Expression_Statement(parser);
    }
}

static void Clox_Compiler_Synchronize(Clox_Parser* parser) {
    parser->panic_mode = false;

    while (parser->current.type != CLOX_TOKEN_EOF) {
        if (parser->previous.type == CLOX_TOKEN_SEMICOLON) {  
            return;
        }

        switch (parser->current.type) {
            case CLOX_TOKEN_CLASS: /* fallthrough */
            case CLOX_TOKEN_FUN: /* fallthrough */
            case CLOX_TOKEN_VAR: /* fallthrough */
            case CLOX_TOKEN_FOR: /* fallthrough */
            case CLOX_TOKEN_IF: /* fallthrough */
            case CLOX_TOKEN_WHILE: /* fallthrough */
            case CLOX_TOKEN_PRINT: /* fallthrough */
            case CLOX_TOKEN_RETURN: {
                return;
            } break;
            default: {
                /* no-op */
            } break;
        }

        Clox_Compiler_Advance(parser);
    }
}

static uint8_t Clox_Compiler_Emit_Identifier_Constant(Clox_Parser* parser) {
    return Clox_Compiler_Make_Constant(parser ,CLOX_VALUE_OBJECT(Clox_String_Create(parser->vm, parser->previous.start, parser->previous.length)));
}

static void Clox_Compiler_Mark_Local_Initialized(Clox_Parser* parser) {
    parser->compiler->locals[parser->compiler->localCount - 1].depth = parser->compiler->scopeDepth;
}

static void Clox_Compiler_Add_Local(Clox_Parser* parser, Clox_Token token) {
    if (parser->compiler->localCount == UINT8_MAX) {
        Clox_Compiler_Error(parser, "Too many local variables in function.");
        return;
    }
    
    Clox_Local* local = &parser->compiler->locals[parser->compiler->localCount++];
    local->name = token;
    local->depth = -1;
}

static bool Clox_Identifiers_Compare(Clox_Token* a, Clox_Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length);
}

static void Clox_Compiler_Declare_Variable(Clox_Parser* parser) {
    if (parser->compiler->scopeDepth == 0) {
        return;
    }

    Clox_Token* name = &parser->previous;
    for (int i = parser->compiler->localCount - 1; i >= 0; i--) {
        Clox_Local* local = &parser->compiler->locals[i];
        if (local->depth != -1 && local->depth < parser->compiler->scopeDepth) {
        break; 
        }

        if (Clox_Identifiers_Compare(name, &local->name) == 0) {
            Clox_Compiler_Error(parser, "Already a variable with this name in this scope.");
        }
    }

    Clox_Compiler_Add_Local(parser, *name);
}

static uint8_t Clox_Compiler_Parse_Variable(Clox_Parser* parser, char const * const message) {
    Clox_Compiler_Consume(parser, CLOX_TOKEN_IDENTIFIER, message);
    
    Clox_Compiler_Declare_Variable(parser);
    if(parser->compiler->scopeDepth > 0) {
        return 0;
    }
    
    return Clox_Compiler_Emit_Identifier_Constant(parser);
}

static void Clox_Compiler_Emit_Define_Variable(Clox_Parser* parser, uint8_t global) {
    if (parser->compiler->scopeDepth > 0) {
        Clox_Compiler_Mark_Local_Initialized(parser);
        return;
    }
    Clox_Compiler_Emit_Bytes(parser, 2, OP_DEFINE_GLOBAL, global);
}

static void Clox_Compiler_Compile_Variable_Declaration(Clox_Parser* parser) {
    uint8_t global = Clox_Compiler_Parse_Variable(parser, "Expect variable name.");

    if (Clox_Compiler_Match(parser, CLOX_TOKEN_EQUAL)) {
        Clox_Compiler_Compile_Expression(parser);
    } else {
        Clox_Compiler_Emit_Byte(parser, OP_NIL);
    }
    Clox_Compiler_Consume(parser, CLOX_TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    Clox_Compiler_Emit_Define_Variable(parser, global);
}

static void Clox_Compiler_Compile_Declaration(Clox_Parser* parser) {
    if (Clox_Compiler_Match(parser, CLOX_TOKEN_VAR)) {
        Clox_Compiler_Compile_Variable_Declaration(parser);
    } else {
        Clox_Compiler_Compile_Statement(parser);
    }
    
    if (parser->panic_mode) {
        Clox_Compiler_Synchronize(parser);
    }
}

static int Clox_Compiler_Resolve_Local(Clox_Parser* parser, Clox_Token* name) {
    for (int i = parser->compiler->localCount - 1; i >= 0; i--) {
        Clox_Local* local = &parser->compiler->locals[i];
        if (Clox_Identifiers_Compare(name, &local->name) == 0) {
            if (local->depth == -1) {
                Clox_Compiler_Error(parser, "Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}


static inline void Clox_Compiler_Compile_Named_Variable(Clox_Parser* parser, bool can_assign) {
    uint8_t getOp, setOp;
    Clox_Token* name = &parser->previous; 
    int arg = Clox_Compiler_Resolve_Local(parser, name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = Clox_Compiler_Emit_Identifier_Constant(parser);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // uint8_t arg = Clox_Compiler_Emit_Identifier_Constant(parser);

    if (can_assign && Clox_Compiler_Match(parser, CLOX_TOKEN_EQUAL)) {
        Clox_Compiler_Compile_Expression(parser);
        Clox_Compiler_Emit_Bytes(parser, 2, setOp, (uint8_t)arg);
    } else {
        Clox_Compiler_Emit_Bytes(parser, 2, getOp, (uint8_t)arg);
    }
}

static inline void Clox_Compiler_Compile_Variable(Clox_Parser* parser, bool can_assign) {
    Clox_Compiler_Compile_Named_Variable(parser, can_assign);
}

static void Clox_Compiler_Compile_And_(Clox_Parser* parser, bool can_assign) {
    (void)can_assign;
    int endJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP_IF_FALSE);

    Clox_Compiler_Emit_Byte(parser, OP_POP);
    Clox_Compiler_Parse_Precendence(parser, CLOX_PRECEDENCE_AND);

    Clox_Compiler_Patch_Jump(parser, endJump);
}

static void Clox_Compiler_Compile_Or_(Clox_Parser* parser, bool can_assign) {
    int elseJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP_IF_FALSE);
    int endJump = Clox_Compiler_Emit_Jump(parser, OP_JUMP);

    Clox_Compiler_Patch_Jump(parser, elseJump);
    Clox_Compiler_Emit_Byte(parser, OP_POP);

    Clox_Compiler_Parse_Precendence(parser, CLOX_PRECEDENCE_OR);
    Clox_Compiler_Patch_Jump(parser, endJump);
}

static inline void Clox_Compiler_Emit_Return(Clox_Parser* parser) {
    Clox_Compiler_Emit_Byte(parser, OP_RETURN);
}

static inline Clox_Function* Clox_Compiler_End(Clox_Parser* parser) {
    Clox_Compiler_Emit_Return(parser);
    return parser->compiler->function;
}



Clox_Function* Clox_Compile_Source_To_Function(Clox_VM* vm, const char* source) {
    Clox_Parser parser = {0};
    Clox_Scanner scanner = Clox_Scanner_New(source);
    Clox_Compiler compiler = {0};
    parser.vm = vm;
    parser.scanner = &scanner;
    parser.compiler = &compiler;
    Clox_Compiler_Init(&parser, &compiler, CLOX_FUNCTION_TYPE_SCRIPT);
    // compiling_chunk = chunk;
    
    Clox_Compiler_Advance(&parser);
    
    while (!Clox_Compiler_Match(&parser, CLOX_TOKEN_EOF)) {
        Clox_Compiler_Compile_Declaration(&parser);
    }

    Clox_Function* fn = Clox_Compiler_End(&parser);

#ifdef CLOX_DEBUG_PRINT_COMPILED_CHUNKS
    if (!parser.had_error) {
        Clox_Chunk_Print(Clox_Compiler_Current_Chunk(&parser), fn->name != NULL ? fn->name->characters : "<script>");
    }
#endif
    return parser.had_error?NULL: fn;
}