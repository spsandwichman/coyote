#pragma once

#include "front.h"

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
    TYPE_PACKED_STRUCT,
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
    alignas(4) u8 kind; \
    u8 visited; \
    u8 emitted; \

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
        Index name; // strpool index
        TypeHandle type;
        u32 offset;
    } fields[];
} TypeNodeRecord;
verify_type(TypeNodeRecord);

typedef struct TypeNodeEnum {
    _TYPE_NODE_BASE
    u16 len;
    TypeHandle underlying;
    struct {
        Index name; // strpool index
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
        Index name; // strpool index
        u64 value;
    } variants[];
} TypeNodeEnum64;
verify_type(TypeNodeEnum64);

typedef struct TypeNodeFunction {
    _TYPE_NODE_BASE
    u16 len;
    TypeHandle return_type;
    struct {
        Index name; // strpool index
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

#define type_node(t) (&an.types.nodes[t])
#define type_as(T, x) ((T*)type_node(x))

// i reuse the concept of a contiguous arena with index handles a lot
// might refactor this into a seperate data structure

TypeHandle type_unalias(TypeHandle t);