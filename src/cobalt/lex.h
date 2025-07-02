#ifndef LEX_H
#define LEX_H

#include "common/type.h"
#include "common/str.h"
#include "common/arena.h"

typedef enum : u16 {
    TOK__INVALID = 0,
    TOK_EOF,
    TOK_IDENT,

    TOK_NEWLINE = '\n',

    TOK_OPEN_PAREN,    // (
    TOK_CLOSE_PAREN,   // )
    TOK_OPEN_BRACE,    // {
    TOK_CLOSE_BRACE,   // }
    TOK_OPEN_BRACKET,  // [
    TOK_CLOSE_BRACKET, // ]

    TOK_SEMICOLON,   // ';
    TOK_QUESTION,    // '?
    TOK_COLON,       // ':
    TOK_COMMA,       // ',
    TOK_DOT,         // '.
    TOK_TILDE,       // '~
    TOK_ARROW,       // ->
    TOK_ELLIPSIS,    // ...
    TOK_COLON_COLON, // ::

    TOK_EQ,     // =
    TOK_ADD_EQ, // +=
    TOK_SUB_EQ, // -=
    TOK_MUL_EQ, // *=
    TOK_DIV_EQ, // /=
    TOK_REM_EQ, // %=
    TOK_AND_EQ, // &=
    TOK_OR_EQ,  // |=
    TOK_XOR_EQ, // ^=
    TOK_LSH_EQ, // <<=
    TOK_RSH_EQ, // >>=

    TOK_ADD,    // +
    TOK_SUB,    // -
    TOK_MUL,    // *
    TOK_DIV,    // /
    TOK_REM,    // %
    TOK_AND,    // &
    TOK_OR,     // |
    TOK_XOR,    // ^
    TOK_LSH,    // <<
    TOK_RSH,    // >>

    TOK_EXCLAM,     // !
    TOK_AND_AND,    // &&
    TOK_OR_OR,      // ||

    TOK_EQ_EQ,      // ==
    TOK_NOT_EQ,     // !=
    TOK_LESS_EQ,    // <=
    TOK_GREATER_EQ, // >=
    TOK_LESS,       // <
    TOK_GREATER,    // >

    TOK_HASH,       // #
    TOK_HASH_HASH,  // ##

    TOK__KW_START = 512,
        TOK_KW_ALIGNAS,
        TOK_KW_ALIGNOF,
        TOK_KW_AUTO,
        TOK_KW_BOOL,
        TOK_KW_BREAK,
        TOK_KW_CASE,
        TOK_KW_CHAR,
        TOK_KW_CONST,
        TOK_KW_CONSTEXPR,
        TOK_KW_CONTINUE,
        TOK_KW_DEFAULT,
        TOK_KW_DO,
        TOK_KW_DOUBLE,
        TOK_KW_ELSE,
        TOK_KW_ENUM,
        TOK_KW_EXTERN,
        TOK_KW_FALSE,
        TOK_KW_FLOAT,
        TOK_KW_FOR,
        TOK_KW_GOTO,
        TOK_KW_IF,
        TOK_KW_INLINE,
        TOK_KW_INT,
        TOK_KW_LONG,
        TOK_KW_NULLPTR,
        TOK_KW_REGISTER,
        TOK_KW_RESTRICT,
        TOK_KW_RETURN,
        TOK_KW_SHORT,
        TOK_KW_SIGNED,
        TOK_KW_SIZEOF,
        TOK_KW_STATIC,
        TOK_KW_STATIC_ASSERT,
        TOK_KW_STRUCT,
        TOK_KW_SWITCH,
        TOK_KW_THREAD_LOCAL,
        TOK_KW_TRUE,
        TOK_KW_TYPEDEF,
        TOK_KW_TYPEOF,
        TOK_KW_TYPEOF_UNQUAL,
        TOK_KW_UNION,
        TOK_KW_UNSIGNED,
        TOK_KW_VOID,
        TOK_KW_VOLATILE,
        TOK_KW_WHILE,
        TOK_KW_ATOMIC,
        TOK_KW_BITINT,
        TOK_KW_COMPLEX,
        TOK_KW_DECIMAL128,
        TOK_KW_DECIMAL64,
        TOK_KW_DECIMAL32,
        TOK_KW_GENERIC,
        TOK_KW_IMAGINARY,
        TOK_KW_NORETURN,
    TOK__KW_END,
    
} TokenKind;

typedef struct {
    char* raw;
    u32 len;
    TokenKind kind;
    bool generated;
} Token;

typedef struct {
    string src;
    usize cursor;
    Arena* arena;
} Lexer;

Token lex_next_raw(Lexer* l);

#endif
