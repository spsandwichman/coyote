#pragma once

#include "jacc.h"

typedef struct Token {
    char* raw;
    u16 len;
    u8 kind;
} Token;

da_typedef(Token);
typedef da(Token) TokenBuf;

typedef struct Lexer {
    TokenBuf* tb;
    string text;
    u32 cursor;
    char current;
} Lexer;

#define _LEX_KEYWORDS_ \
    T(AND), \
    T(BREAK), \
    T(BYTE), \
    T(CAST), \
    T(CONTAINEROF), \
    T(CONTINUE), \
    T(DO), \
    T(ELSE), \
    T(ELSEIF), \
    T(END), \
    T(ENUM), \
    T(EXTERN), \
    T(FALSE), \
    T(FN), \
    T(FNPTR), \
    T(GOTO), \
    T(IF), \
    T(IN), \
    T(INT), \
    T(LEAVE), \
    T(LONG), \
    T(NOT), \
    T(NULLPTR), \
    T(OR), \
    T(OUT), \
    T(PACKED), \
    T(PUBLIC), \
    T(RETURN), \
    T(SIZEOF), \
    T(OFFSETOF), \
    T(SIZEOFVALUE), \
    T(STRUCT), \
    T(THEN), \
    T(TO), \
    T(TRUE), \
    T(TYPE), \
    T(UBYTE), \
    T(UINT), \
    T(ULONG), \
    T(UNION), \
    T(VOID), \
    T(WHILE), \
    T(BARRIER), \
    T(NOTHING), \
    T(PROBE), \
    T(EXPORT), \
    T(PRIVATE), \
    T(UQUAD), \
    T(QUAD), \


enum {
    TOK__INVALID,
    TOK_EOF,

    TOK_OPEN_PAREN,     // (
    TOK_CLOSE_PAREN,    // )
    TOK_OPEN_BRACE,     // {
    TOK_CLOSE_BRACE,    // }
    TOK_OPEN_BRACKET,   // [
    TOK_CLOSE_BRACKET,  // ]

    TOK_COLON,      // :
    TOK_COMMA,      // ,
    TOK_EQ_EQ,      // ==
    TOK_NOT_EQ,     // !=
    TOK_AND,        // &=
    TOK_OR,         // |
    TOK_LESS,       // <
    TOK_GREATER,    // >
    TOK_LESS_EQ,    // <=
    TOK_GREATER_EQ, // >=
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_MUL,        // *
    TOK_DIV,        // /
    TOK_MOD,        // %
    TOK_DOT,        // .
    TOK_AT,         // @
    TOK_DOLLAR,     // $
    TOK_LSHIFT,     // <<
    TOK_RSHIFT,     // >>
    TOK_TILDE,      // ~
    TOK_VARARG,     // ...
    TOK_CARET,      // ^

    TOK_EQ,         // =
    TOK_PLUS_EQ,    // +=
    TOK_MINUS_EQ,   // -=
    TOK_MUL_EQ,     // *=
    TOK_DIV_EQ,     // /=
    TOK_MOD_EQ,     // %=
    TOK_AND_EQ,     // &=
    TOK_OR_EQ,      // |=
    TOK_DOLLAR_EQ,  // $=
    TOK_LSHIFT_EQ,  // <<=
    TOK_RSHIFT_EQ,  // >>=

    TOK_IDENTIFIER,

    TOK_CHAR,
    TOK_STRING,

    TOK_NUMERIC,

    #define T(ident) TOK_KEYWORD_##ident
        _LEX_KEYWORDS_
    #undef T
};

TokenBuf lex_tokenize(SourceFile* src);