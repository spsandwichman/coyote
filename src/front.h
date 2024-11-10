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
    string path;
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

extern const char* token_kind[_TOK_COUNT];

TokenBuf lex_tokenize(SourceFile* src);

void lex_advance(Lexer* l);
void lex_advance_n(Lexer* l, int n);
void lex_skip_whitespace(Lexer* l);

void preproc_dispatch(Lexer* l);

enum {
    PN_INVALID = 0,

    PN_EXPR_INT,
    PN_EXPR_STRING,
    PN_EXPR_BOOL_TRUE,
    PN_EXPR_BOOL_FALSE,
    PN_EXPR_NULLPTR,

    PN_EXPR_IDENT,

    PN_EXPR_COMPOUND, // list
    PN_COMPOUND_ITEM, // bin

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
        PN_EXPR_ADDRESS,
        PN_EXPR_NEGATE,
        PN_EXPR_BIT_NOT,
        PN_EXPR_BOOL_NOT,
        PN_EXPR_SIZEOF,
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

    PN_TYPE_POINTER, // bin: lhs
    PN_TYPE_ARRAY,   // bin: lhs = type, rhs = len

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

    PN_STMT_PROBE,
    PN_STMT_BARRIER,
    PN_STMT_BREAK,
    PN_STMT_CONTINUE,
    PN_STMT_LEAVE,
    PN_STMT_GOTO,       // ident = main_token + 1
    PN_STMT_LABEL,      // ident = main_token + 1
    PN_STMT_RETURN,     // lhs = expr

    PN_STMT_WHILE,  // lhs = cond, rhs = PNExtraList
    PN_STMT_IF,     // lhs = cond, rhs = PNExtraList
    PN_STMT_IFELSE, // lhs = cond, rhs = PNExtraIfElseStmt

    // identifiers can be derived from this
    PN_STMT_DECL,        // lhs = type, rhs = value
    PN_STMT_PRIVATE_DECL,// lhs = type, rhs = value
    PN_STMT_EXTERN_DECL, // lhs = type, rhs = value
    PN_STMT_PUBLIC_DECL, // lhs = type, rhs = value
    PN_STMT_EXPORT_DECL, // lhs = type, rhs = value

    PN_STMT_FN_DECL,         // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_PUBLIC_FN_DECL,  // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_PRIVATE_FN_DECL, // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_EXPORT_FN_DECL,  // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_EXTERN_FN,       // lhs = PNExtraFnProto

    PN_STMT_TYPE_DECL,          // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_STRUCT_DECL,        // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_STRUCT_PACKED_DECL, // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_UNION_DECL,         // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_ENUM_DECL,          // lhs = type, rhs = PNExtraList
    PN_STMT_FNPTR_DECL,         // lhs = PNExtraFnProto

    PN_IN_PARAM,  // lhs = ident (token index), rhs = type
    PN_OUT_PARAM, // lhs = ident (token index), rhs = type
};

typedef u32 Index;
#define NULL_INDEX (Index)0

typedef struct ParseNode ParseNode;
typedef struct ParseTree {
    Index head;
    struct {
        u8* kinds; // parallel array of parse-node kinds
        ParseNode* at;
        u32 len;
        u32 cap;
    } nodes;
    struct {
        Index* at;
        u32 len;
        u32 cap;
    } extra;
} ParseTree;

typedef struct ParseNode {
    u32 main_token;
    union {
        struct {
            Index lhs;
            Index rhs;
        } bin;
        struct {
            Index items; // into extra array
            u32 len;
        } list;
    };
} ParseNode;

#define verify_extra(T) static_assert(sizeof(T) % sizeof(Index) == 0)

typedef struct PNExtraList {
    u32 len;
    Index items[];
} PNExtraList;
verify_extra(PNExtraList);

typedef struct PNExtraFnProto {
    Index fnptr_type; // (T)
    Index identifier; // token index
    Index return_type; // : T

    u32 params_len;
    Index params[]; // list of PN_IN_PARAM / PN_OUT_PARAM
} PNExtraFnProto;
verify_extra(PNExtraFnProto);

typedef struct PNExtraIfElseStmt {
    Index block;
    Index else_branch;
} PNExtraIfElseStmt;
verify_extra(PNExtraIfElseStmt);

#undef verify_extra

void emit_report(bool error, string source, string path, string highlight, char* message, va_list varargs);

ParseTree parse_file(TokenBuf tb);

Index parse_type();
Index parse_base_type();
Index parse_expr();

Index parse_stmt();
Index parse_decl();
Index parse_fn_decl();
Index parse_var_decl(bool is_extern);
Index parse_type_decl();