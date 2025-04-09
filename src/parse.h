#ifndef PARSE_H
#define PARSE_H

#include "lex.h"

// ----------------- TYPES AND SHIT ----------------- 

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
    TYPE_ENUM,
    TYPE_FNPTR,
    TYPE_FN,
};

typedef u16 TypeIndex;
typedef struct {u32 _;} TypeBufSlot;

typedef struct {
    u8 kind;
} TypeBase;

typedef struct {
    u8 kind;
    TypeIndex to;
} TypePtr;
static_assert(sizeof(TypePtr) == sizeof(TypeBufSlot));
// important for ptr cache trick

typedef struct {
    TypeIndex type;
    CompactString field;
} TypeRecordField;

typedef struct {
    u8 kind;
    u8 len; // should never be a limitation O.O
    TypeIndex fields[];
} TypeRecord;

// pointers to records are always preallocated
// before the record's slot, for fast access 
// (and so only one pointer type is ever needed)
#define record_ptr_cache(typeindex) (typeindex - 1)

// ------------------- PARSE/SEMA ------------------- 

typedef struct {
    TypeIndex type; // expression type
} Expr;

typedef struct {
    
} Entity;

#endif // PARSE_H