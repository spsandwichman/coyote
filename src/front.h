#pragma once

#include "coyote.h"
#include "crash.h"

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
    T(AND) \
    T(OR) \
    T(BREAK) \
    T(BYTE) \
    T(CAST) \
    T(CONTAINEROF) \
    T(CONTINUE) \
    T(DO) \
    T(ELSE) \
    T(ELSEIF) \
    T(END) \
    T(ENUM) \
    T(EXTERN) \
    T(FALSE) \
    T(FN) \
    T(FNPTR) \
    T(GOTO) \
    T(IF) \
    T(IN) \
    T(INT) \
    T(LEAVE) \
    T(LONG) \
    T(NOT) \
    T(NULLPTR) \
    T(OUT) \
    T(PACKED) \
    T(PUBLIC) \
    T(RETURN) \
    T(SIZEOF) \
    T(OFFSETOF) \
    T(SIZEOFVALUE) \
    T(STRUCT) \
    T(THEN) \
    T(TO) \
    T(TRUE) \
    T(TYPE) \
    T(UBYTE) \
    T(UINT) \
    T(ULONG) \
    T(UNION) \
    T(VOID) \
    T(WHILE) \
    T(BARRIER) \
    T(NOTHING) \
    T(EXPORT) \
    T(PRIVATE) \
    T(UQUAD) \
    T(QUAD) \
    T(UWORD) \
    T(WORD) \

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
    TOK_DOT,        // .
    TOK_AT,         // @
    TOK_TILDE,      // ~
    TOK_VARARG,     // ...
    TOK_CARET,      // ^

    // this must be the same order as the parse nodes
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
    
    // DO NOT REARRANGE THESE - important for parser trick
    // this must be the same order as their parse nodes
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_MUL,        // *
    TOK_DIV,        // /
    TOK_MOD,        // %
    TOK_LSHIFT,     // <<
    TOK_RSHIFT,     // >>
    TOK_OR,         // |
    TOK_AND,        // &
    TOK_DOLLAR,     // $
    TOK_NOT_EQ,     // !=
    TOK_EQ_EQ,      // ==
    TOK_LESS,       // <
    TOK_GREATER,    // >
    TOK_LESS_EQ,    // <=
    TOK_GREATER_EQ, // >=

    _TOK_KEYWORDS_BEGIN,

    #define T(ident) TOK_KEYWORD_##ident,
        _LEX_KEYWORDS_
    #undef T

    _TOK_KEYWORDS_END,

    _TOK_COUNT
};

extern const char* token_kind[_TOK_COUNT];

void lex_init_keyword_table();
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

    PN_COMPOUND_EXPR, // lhs = PNExtraList
    PN_COMPOUND_ITEM, // rhs = indexer, lhs = value

    
    // DO NOT REARRANGE THESE - important for sema trick
    // {
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
    // }

    _PN_UNOP_BEGIN,
        PN_EXPR_DEREF,
        PN_EXPR_ADDRESS,
        PN_EXPR_NEGATE,
        PN_EXPR_BIT_NOT,
        PN_EXPR_BOOL_NOT,
        PN_EXPR_SIZEOF,
        PN_EXPR_SIZEOFVALUE,
    _PN_UNOP_END,

    PN_OUT_ARGUMENT, // lhs = value
    PN_EXPR_CALL, // lhs = callee, rhs = PNExtraList

    PN_EXPR_OFFSETOF,
    PN_EXPR_CAST,

    PN_EXPR_SELECTOR, // lhs = subexpr, rhs = ident (token index)
    PN_EXPR_INDEX,    // lhs = subexpr, rhs = index expr

    // bin: CONTAINEROF lhs TO rhs
    // rhs must be a selector
    PN_EXPR_CONTAINEROF,

    _PN_TYPE_SIMPLE_BEGIN,
        PN_TYPE_WORD,
        PN_TYPE_UWORD,
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

    PN_STMT_BARRIER,
    PN_STMT_BREAK,
    PN_STMT_CONTINUE,
    PN_STMT_LEAVE,
    PN_STMT_GOTO,       // lhs = ident (token index)
    PN_STMT_LABEL,      // lhs = ident (token index)
    PN_STMT_RETURN,     // lhs = expr

    PN_STMT_WHILE,    // lhs = cond, rhs = PNExtraList
    PN_STMT_IF,       // lhs = cond, rhs = PNExtraList
    PN_STMT_IFELSE,   // lhs = cond, rhs = PNExtraIfElseStmt
    PN_STMT_IFELSEIF, // lhs = cond, rhs = PNExtraIfElseStmt
    // ^ this is the same thing as PN_STMT_IFELSE, except that .else_branch
    // in PNExtraIfElseStmt points to a ParseNode instead of a PNExtraList

    // DO NOT REARRANGE THESE - important for parser trick
    // {
    PN_STMT_DECL,        // lhs = type, rhs = value
    PN_STMT_PRIVATE_DECL,// lhs = type, rhs = value
    PN_STMT_PUBLIC_DECL, // lhs = type, rhs = value
    PN_STMT_EXPORT_DECL, // lhs = type, rhs = value
    PN_STMT_EXTERN_DECL, // lhs = type

    PN_STMT_FN_DECL,         // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_PRIVATE_FN_DECL, // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_PUBLIC_FN_DECL,  // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_EXPORT_FN_DECL,  // lhs = PNExtraFnProto, rhs = PNExtraList
    PN_STMT_EXTERN_FN_DECL,  // lhs = PNExtraFnProto
    // }

    PN_STMT_TYPE_DECL,          // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_STRUCT_DECL,        // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_STRUCT_PACKED_DECL, // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_UNION_DECL,         // lhs = ident (token index), rhs = PNExtraList
    PN_STMT_ENUM_DECL,          // lhs = type, rhs = PNExtraList
    PN_STMT_FNPTR_DECL,         // lhs = PNExtraFnProto

    PN_FIELD,   // lhs = ident (token index), rhs = type
    PN_VARIANT, // lhs = ident (token index), rhs = value

    PN_IN_PARAM,  // lhs = ident (token index), rhs = type
    PN_OUT_PARAM, // lhs = ident (token index), rhs = type
    PN_VAR_PARAM, // lhs = argv (token index), rhs = argc (token index)
};

typedef u32 Index;
#define NULL_INDEX (Index)0

typedef struct ParseNode ParseNode;
typedef struct ParseTree {
    Index* decls;
    u32 len;

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
    Index lhs;
    Index rhs;
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
    Index params[]; // list of PN_IN_PARAM / PN_OUT_PARAM / PN_VAR_PARAM
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
Index parse_binary(isize precedence);
#define parse_expr() parse_binary(0)

Index parse_stmt();
Index parse_decl();
Index parse_fn_decl();
Index parse_var_decl(bool is_extern);
Index parse_type_decl();

ParseNode* parse_node(Index i);
u8 parse_node_kind(Index i);

typedef u32 TypeHandle;

enum {
    TYPE_VOID = 0, // simple type kinds double as their TypeHandles
    TYPE_I8,
    TYPE_U8,
    TYPE_I16,
    TYPE_U16,
    TYPE_I32,
    TYPE_U32,
    TYPE_I64,
    TYPE_U64,

    TYPE_INT_CONSTANT,

    _TYPE_SIMPLE_END, // any types after this are indexes

    TYPE_POINTER,
    TYPE_ARRAY,

    TYPE_STRUCT,
    TYPE_UNION,

    TYPE_ENUM,
    // enum with the capacity for 64 bit values
    // same as TYPE_ENUM, but has more efficent storage for small values
    TYPE_ENUM64,
    
    TYPE_FNPTR,
    // same as above, except it has varargs.
    TYPE_VARIADIC_FNPTR,

    // this is how unknown types get constructed.
    TYPE_ALIAS_UNDEF, // unknown
    TYPE_ALIAS, // known
};

typedef u32 EntityHandle;
typedef u32 TypeHandle;

#define TYPEHANDLE_BITS 25
#define TYPE_MAX ((1ull << TYPEHANDLE_BITS) - 1)

#define verify_type(T) static_assert(sizeof(T) % sizeof(TypeNode) == 0)

#define _TYPE_NODE_BASE \
    alignas(4) u8 kind;

typedef struct TypeNode {
    _TYPE_NODE_BASE
} TypeNode;
static_assert(sizeof(TypeNode) == sizeof(u32));

typedef struct TypeNodeAlias {
    _TYPE_NODE_BASE
    TypeHandle subtype;
    EntityHandle entity;
} TypeNodeAlias;
verify_type(TypeNodeAlias);

typedef struct TypeNodePointer {
    _TYPE_NODE_BASE
    TypeHandle subtype;
} TypeNodePointer;
verify_type(TypeNodePointer);

#define ARRAY_UNKNOWN_LEN (UINT32_MAX)
typedef struct TypeNodeArray {
    _TYPE_NODE_BASE
    TypeHandle subtype;
    u32 len;
} TypeNodeArray;
verify_type(TypeNodeArray);

typedef struct TypeNodeRecord {
    _TYPE_NODE_BASE
    u8 align;
    u16 len;
    u32 size; // in bytes

    // since pointers to structs are VERY common,
    // cache the pointer handle inside the struct
    TypeHandle pointer_cache;
    struct {
        Index name_token; // token index
        TypeHandle type;
    } fields[];
} TypeNodeRecord;
verify_type(TypeNodeRecord);

typedef struct TypeNodeEnum {
    _TYPE_NODE_BASE
    u16 len;
    TypeHandle underlying;
    struct {
        Index name_token; // token index
        u32 value;
    } variants[];
} TypeNodeEnum;
verify_type(TypeNodeEnum);

// encoding 64-bit capacity enums with different, less efficient value storage
typedef struct TypeNodeEnum64 {
    _TYPE_NODE_BASE
    u16 len;
    TypeHandle underlying;
    struct {
        Index name_token; // token index
        u64 value;
    } variants[];
} TypeNodeEnum64;
verify_type(TypeNodeEnum64);

typedef struct TypeNodeFunction {
    _TYPE_NODE_BASE
    u16 len;
    TypeHandle return_type;
    struct {
        Index name_token; // token index
        TypeHandle type : TYPEHANDLE_BITS;

        // this is really a bool, but its u32 because it needs to be 
        // compatible with TypeHandle for bit field to work
        // fuck you C
        u32 out : 1;
    } params[];
} TypeNodeFunction;
verify_type(TypeNodeFunction);

// variadic functions need to store 8 more bytes of information by default
// BUT they're much rarer than regular functions, so its worth it to store them differently
// the shit i do for memory/cache efficiency bruh
typedef struct TypeNodeVariadicFunction {
    _TYPE_NODE_BASE
    u16 len;
    TypeHandle return_type;
    Index argv_token; // argv is ^^VOID
    Index argc_token; // argc is UWORD
    struct {
        Index name_token; // token index
        TypeHandle type : 31;
        u32 out : 1;
    } params[];
} TypeNodeVariadicFunction;
verify_type(TypeNodeVariadicFunction);

#undef verify_type

const char* type_name(TypeHandle t);


enum {
    ENTKIND_VAR = 0, // default, variable
    ENTKIND_FN,
    ENTKIND_TYPENAME,
};

enum {
    STORAGE_LOCAL,   // local variable
    STORAGE_OUT,     // out parameter

    STORAGE_EXTERN,  // not defined here

    // globals
    STORAGE_PUBLIC,  // static export
    STORAGE_EXPORT,  // dynamic global
    STORAGE_PRIVATE, //

};

typedef u32 EntityHandle;

#define MAX_STRING ((1u << 27) - 1)

// a named thing.
typedef struct Entity {
    Index decl_index; // SemaStmt
    Index ident_string : 27; // index into strpool
    TypeHandle type : TYPEHANDLE_BITS;
    u32 kind : 2;
    u32 storage : 3;
    u32 is_global : 1;
} Entity;

// a scope.
typedef struct EntityTable EntityTable;
typedef struct EntityTable {
    EntityTable* parent; // parent scope

    EntityHandle* entities;
    u32 cap;
} EntityTable;

Entity* entity_get(EntityHandle e);

typedef struct SemaExpr {
    Index parse_node;
    TypeHandle type : TYPEHANDLE_BITS;
    u32 kind : 6;
} SemaExpr;
static_assert(sizeof(SemaExpr) == 8);

typedef struct SemaExprEntity {
    SemaExpr base;
    EntityHandle entity;
} SemaExprEntity;

typedef struct SemaExprInteger {
    SemaExpr base;
    u32 value;
} SemaExprInteger;

typedef struct SemaExprInteger64 {
    SemaExpr base;
    u32 value_low;
    u32 value_high;
} SemaExprInteger64;

typedef struct SemaExprBinop {
    SemaExpr base;
    Index lhs;
    Index rhs;
} SemaExprBinop;

typedef struct SemaExprUnop {
    SemaExpr base;
    Index sub;
} SemaExprUnop;

typedef struct SemaExprCall {
    SemaExpr base;
    Index callee;
    u32 len;
    Index params[];
} SemaExprCall;

typedef struct SemaExprCompound {
    SemaExpr base;
} SemaExprCompound;

enum {
    SE_ENTITY,
    SE_INTEGER,
    SE_INTEGER64,

    SE_STRING,

    SE_COMPOUND,

    // DO NOT REARRANGE THESE - important for sema trick
    // {
    SE_ADD,
    SE_SUB,
    SE_MUL,
    SE_DIV,
    SE_MOD,
    SE_LSH,
    SE_RSH,
    SE_BIT_OR,
    SE_BIT_AND,
    SE_BIT_XOR,
    SE_NEQ,
    SE_EQ,
    SE_LESS,
    SE_GREATER,
    SE_LESS_EQ,
    SE_GREATER_EQ,
    SE_BOOL_OR,
    SE_BOOL_AND,
    SE_INDEX,
    // }

    // SemaExprUnop
    SE_BOOL_NOT,
    SE_DEREF, // address dereference
    SE_ADDRESS, // take address
    SE_NEG,
    SE_BIT_NOT,

    SE_CAST,
    SE_IMPLICIT_CAST, // same as SE_CAST but only inserted by the compiler

    // lmfao
    SE_PASS_OUT,

    SE_CALL,
};

#define _SEMA_STMT_BASE \
    Index parse_node; \
    u8 kind;

typedef struct SemaStmt {
    _SEMA_STMT_BASE
} SemaStmt;

typedef struct SemaStmtDecl {
    _SEMA_STMT_BASE
    EntityHandle entity;
    Index value;
} SemaStmtDecl;

typedef struct SemaStmtGroup {
    _SEMA_STMT_BASE
    u16 len;
    Index stmts[];
} SemaStmtGroup;

enum {
    SS_VAR_DECL,
    SS_FN_DECL,
    SS_STMT_GROUP,
};

typedef struct Analyzer {
    struct {
        u32* items;
        u32 len;
        u32 cap;
    } exprs;

    struct {
        u32* items;
        u32 len;
        u32 cap;
    } stmts;

    struct {
        Entity* items;
        u32 len;
        u32 cap;
    } entities;

    struct {
        TypeNode* nodes;
        u32 len;
        u32 cap;
    } types;

    struct { // string data arena.
        u8* chars;
        u32 len;
        u32 cap;
    } strings;

    EntityTable* global;

    ParseTree pt;
    TokenBuf  tb;

    TypeHandle max_int;
    TypeHandle max_uint; // TODO extract into TargetInfo struct or smth

    struct {
        TypeHandle return_type;
    } fn_context;
} Analyzer;

Analyzer sema_analyze(ParseTree pt, TokenBuf tb);

// i reuse the concept of a contiguous arena with index handles a lot
// might refactor this into a seperate data structure

void report_token(bool error, TokenBuf* tb, u32 token_index, char* message, ...);