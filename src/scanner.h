#ifndef CLOX_SCNANER_H_INCLUDED
#define CLOX_SCNANER_H_INCLUDED

#include "common.h"

typedef struct  {
    const char* start;
    const char* current;
    int line;
} Clox_Scanner;

typedef enum {
    // Single-character tokens.
    CLOX_TOKEN_LEFT_PAREN,
    CLOX_TOKEN_RIGHT_PAREN,
    CLOX_TOKEN_LEFT_BRACE,
    CLOX_TOKEN_RIGHT_BRACE,
    CLOX_TOKEN_COMMA,
    CLOX_TOKEN_DOT,
    CLOX_TOKEN_MINUS,
    CLOX_TOKEN_PLUS,
    CLOX_TOKEN_SEMICOLON,
    CLOX_TOKEN_SLASH,
    CLOX_TOKEN_STAR,
    // One or two character tokens.
    CLOX_TOKEN_BANG,
    CLOX_TOKEN_BANG_EQUAL,
    CLOX_TOKEN_EQUAL,
    CLOX_TOKEN_EQUAL_EQUAL,
    CLOX_TOKEN_GREATER,
    CLOX_TOKEN_GREATER_EQUAL,
    CLOX_TOKEN_LESS,
    CLOX_TOKEN_LESS_EQUAL,
    // Literals.
    CLOX_TOKEN_IDENTIFIER,
    CLOX_TOKEN_STRING,
    CLOX_TOKEN_NUMBER,
    // Keywords.
    CLOX_TOKEN_AND,
    CLOX_TOKEN_CLASS,
    CLOX_TOKEN_ELSE,
    CLOX_TOKEN_FALSE,
    CLOX_TOKEN_FOR,
    CLOX_TOKEN_FUN,
    CLOX_TOKEN_IF,
    CLOX_TOKEN_NIL,
    CLOX_TOKEN_OR,
    CLOX_TOKEN_PRINT,
    CLOX_TOKEN_RETURN,
    CLOX_TOKEN_SUPER,
    CLOX_TOKEN_THIS,
    CLOX_TOKEN_TRUE,
    CLOX_TOKEN_VAR,
    CLOX_TOKEN_WHILE,

    // Special
    CLOX_TOKEN_ERROR,
    CLOX_TOKEN_EOF
} Clox_Token_Type;

typedef struct Clox_Token Clox_Token;
struct Clox_Token {
    Clox_Token_Type type;
    const char* start;
    int length;
    int line;
};

Clox_Scanner Clox_Scanner_New(const char* source);
Clox_Token Clox_Scanner_Get_Token(Clox_Scanner* scanner);

#endif // CLOX_SCNANER_H_INCLUDED