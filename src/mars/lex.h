#ifndef MARS_LEX_H
#define MARS_LEX_H

#include "common/type.h"
#include "common/str.h"

typedef enum : u8 {
    TOK_INVALID,

    TOK_EOF,

    TOK_OPEN_PAREN,    // (
    TOK_CLOSE_PAREN,   // )
    TOK_OPEN_BRACE,    // {
    TOK_CLOSE_BRACE,   // }
    TOK_OPEN_BRACKET,  // [
    TOK_CLOSE_BRACKET, // ]

    TOK_SEMICOLON,   // ;
    TOK_QUESTION,    // ?
    TOK_COLON,       // :
    TOK_COMMA,       // ,
    TOK_DOT,         // .
    TOK_COLON_COLON, // ::
    TOK_RANGE,       // ..
    TOK_RANGE_EQ,    // ..=
    TOK_UNDERSCORE, // _

    TOK_EQ,     // =
    TOK_ADD_EQ, // +=
    TOK_SUB_EQ, // -=
    TOK_MUL_EQ, // *=
    TOK_DIV_EQ, // /=
    TOK_REM_EQ, // %=
    TOK_AND_EQ, // &=
    TOK_OR_EQ,  // |=
    TOK_XOR_EQ, // ~=
    TOK_LSH_EQ, // <<=
    TOK_RSH_EQ, // >>=

    TOK_ADD,    // +
    TOK_SUB,    // -
    TOK_MUL,    // *
    TOK_DIV,    // /
    TOK_REM,    // %
    TOK_AND,    // &
    TOK_OR,     // |
    TOK_TIDLE,  // ~
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

    TOK_IBIT, // arbitrary bit size up to isize
    TOK_UBIT, // arbitrary bit size up to usize

    TOK_KW_MODULE,
    TOK_KW_BUILTIN,
    TOK_KW_COMMON,
    TOK_KW_THREADLOCAL,
    TOK_KW_EXTERN,
    TOK_KW_PUB,
    TOK_KW_CONST,
    TOK_KW_DEF,
    TOK_KW_LET,
    TOK_KW_MUT,
    TOK_KW_TRUE,
    TOK_KW_FALSE,
    TOK_KW_NULL,
    TOK_KW_UNDEF,
    TOK_KW_UNREACHABLE,
    TOK_KW_DEFER,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_WHILE,
    TOK_KW_FOR,
    TOK_KW_IN,
    TOK_KW_AS,
    TOK_KW_WHERE,
    TOK_KW_INLINE,
    TOK_KW_PACKED,
    TOK_KW_NOALIAS,
    TOK_KW_ALIGN,
    TOK_KW_VOID,
    TOK_KW_BOOL,
    TOK_KW_F32,
    TOK_KW_F64,
    TOK_KW_TYPE,
    TOK_KW_STRUCT,
    TOK_KW_UNION,
    TOK_KW_ENUM,
    TOK_KW_NORETURN,

    TOK_KW_ISIZE,
    TOK_KW_USIZE,

} TokenKind;

typedef struct {
    TokenKind kind;
    u32 len;
    const char* data;
} Token;

typedef struct {
    string path;
    string src;
    u32 cursor;
} Lexer;

#endif