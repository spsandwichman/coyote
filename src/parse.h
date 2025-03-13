#ifndef PARSE_H
#define PARSE_H

#include "lex.h"

enum TypeKind {
    TYPE_VOID,

    TYPE_BYTE,
    TYPE_UBYTE,
    TYPE_INT,
    TYPE_UINT,
    TYPE_LONG,
    TYPE_ULONG,
    TYPE_QUAD,
    TYPE_UQUAD,

    TYPE_PTR,
    TYPE_STRUCT,
    TYPE_STRUCT_PACKED,
    TYPE_UNION,
    TYPE_FNPTR,
};

typedef u16 TypeIndex;

typedef struct {
    u8 kind;
} TypeBase;

typedef struct {
    u8 kind;
    TypeIndex to;
} TypePtr;

enum AstKind {
    AST_IDENT,
};

typedef struct {
    u8 kind;
} Ast;

#endif // PARSE_H