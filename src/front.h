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
    _TOK_INVALID,
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

    _TOK_COUNT
};

TokenBuf lex_tokenize(SourceFile* src);

void lex_advance(Lexer* l);
void lex_advance_n(Lexer* l, int n);
void lex_skip_whitespace(Lexer* l);

void preproc_dispatch(Lexer* l);

enum {
    PN_INVALID = 0,

    _PN_BINOP_BEGIN,
        PN_EXPR_ADD,
        PN_EXPR_SUB,
        PN_EXPR_MUL,
        PN_EXPR_DIV,
        PN_EXPR_MOD,
        PN_EXPR_LSHIFT,
        PN_EXPR_RSHIFT,
        PN_EXPR_BIT_OR,
        PN_EXPR_BIT_AND,
        PN_EXPR_BIT_XOR,
        PN_EXPR_NOT_EQ,
        PN_EXPR_EQ,
        PN_EXPR_LESS,
        PN_EXPR_GREATER,
        PN_EXPR_LESS_EQ,
        PN_EXPR_GREATER_EQ,
        PN_EXPR_BOOL_AND,
        PN_EXPR_BOOL_OR,
    _PN_BINOP_END,

    _PN_UNOP_BEGIN,
        PN_EXPR_DEREF,
        PN_EXPR_NEGATE,
        PN_EXPR_BIT_NOT,
        PN_EXPR_BOOL_NOT,
        PN_EXPR_SIZEOFVALUE,
    _PN_UNOP_END,

    // bin
    PN_EXPR_OFFSETOF,
    PN_EXPR_SELECTOR,
    PN_EXPR_CAST,

    // bin: CONTAINEROF lhs TO rhs
    // rhs must be a selector
    PN_EXPR_CONTAINEROF,

    _PN_TYPE_SIMPLE_BEGIN,
        PN_TYPE_QUAD,
        PN_TYPE_UQUAD,
        PN_TYPE_LONG,
        PN_TYPE_ULONG,
        PN_TYPE_INT,
        PN_TYPE_UINT,
        PN_TYPE_BYTE,
        PN_TYPE_UBYTE,
        PN_TYPE_VOID,
    _PN_TYPE_SIMPLE_END,

    PN_TYPE_POINTER, // un
    PN_TYPE_ARRAY,   // bin

    PN_EXPR_INT,
    PN_EXPR_FLOAT,
    PN_EXPR_STRING,
    PN_EXPR_BOOL_TRUE,
    PN_EXPR_BOOL_FALSE,
    PN_EXPR_NULLPTR,

    PN_EXPR_IDENT,

    _PN_STMT_ASSIGN_BEGIN, // bin
        PN_STMT_ASSIGN,
        PN_STMT_ASSIGN_ADD,
        PN_STMT_ASSIGN_SUB,
        PN_STMT_ASSIGN_MUL,
        PN_STMT_ASSIGN_DIV,
        PN_STMT_ASSIGN_MOD,
        PN_STMT_ASSIGN_AND,
        PN_STMT_ASSIGN_OR,
        PN_STMT_ASSIGN_XOR,
        PN_STMT_ASSIGN_LSHIFT,
        PN_STMT_ASSIGN_RSHIFT,
    _PN_STMT_ASSIGN_END, // bin

    PN_STMT_BLOCK,  // list
    PN_STMT_IF,     // bin
    PN_STMT_WHILE,  // bin
    PN_STMT_GOTO,   // un
    PN_STMT_PROBE,
    PN_STMT_BARRIER,
    PN_STMT_BREAK,
    PN_STMT_CONTINUE,
    PN_STMT_LEAVE,
    PN_STMT_RETURN,     // un
    PN_STMT_LABEL,      // un
    PN_STMT_TYPEDECL,   // bin

    PN_STMT_IF,     // bin
    PN_STMT_IFELSE, // un, .sub = condition
    PN_STMT_WHILE,  // bin

};

typedef u32 ParseNodeIndex;
typedef u32 ParseExtraIndex;
#define PN_NULL (ParseNodeIndex)0

typedef struct PNExtraIfStmt {
    ParseNodeIndex block;
    ParseNodeIndex else_branch;
} PNExtraIfStmt;

typedef struct ParseTree {

    ParseNodeIndex head;

    struct {
        u8* kinds; // parallel array of parse-node kinds
        ParseNode* at;
        u32 len;
        u32 cap;
    } nodes;

    struct {
        ParseNodeIndex* at;
        u32 len;
        u32 cap;
    } extra;
} ParseTree;

typedef struct ParseNode {
    u32 main_token;

    union {
        struct {
            ParseNodeIndex lhs;
            ParseNodeIndex rhs;
        } bin;

        struct {
            ParseNodeIndex sub;
            ParseExtraIndex extra;
        } un;

        struct {
            ParseExtraIndex items;
            u32 len;
        } list;
    };
} ParseNode;