#ifndef PARSE_H
#define PARSE_H

#include "lex.h"

#include "iron/iron.h"

// ----------------- TYPES AND SHIT ----------------- 

typedef enum : u8 {
    TY__INVALID,

    TY_VOID,

    TY_BYTE,    // i8
    TY_UBYTE,   // u8
    TY_INT,     // i16
    TY_UINT,    // u16
    TY_LONG,    // i32
    TY_ULONG,   // u32
    TY_QUAD,    // i64
    TY_UQUAD,   // u64

    TY_PTR,
    TY_STRUCT,
    TY_STRUCT_PACKED,
    TY_UNION,
    TY_ENUM,
    TY_FNPTR,
    TY_FN,
} TyKind;

typedef u16 TyIndex;
typedef struct {u32 _;} TyBufSlot;

typedef struct {
    TyKind kind;
} TyBase;

typedef struct {
    u8 kind;
    TyIndex to;
} TyPtr;

typedef struct {
    TyIndex type;
    u16 offset;
    CompactString name;
} TyRecordField;

#define RECORD_MAX_FIELDS UINT8_MAX
typedef struct {
    u8 kind;
    u8 len; // should never be a limitation O.O
    u16 size;
    u8 align;
    TyRecordField fields[];
} TypeRecord;

// ------------------- PARSE/SEMA ------------------- 

typedef struct {
    TyIndex ty; // expression type
    FeTy fe_ty; // iron type
    FeInst* fe_value;
} Expr;

typedef struct {
    struct {
        FeModule* m; // module/compilation unit
        FeFunc* f; // current function
        FeBlock* b; // current block
    } fe;

    Token current;
    Token* tokens;
    u32 tokens_len;
    u32 cursor;
} Parser;

typedef enum : u8 {
    GEN_VAL, // generate ir for the value of this expression
    GEN_PTR, // generate ir for the address of this expression.
    GEN_NONE,// dont generate ir for this expression, just analyze it.
} GenMode;

typedef struct {
} Entity;

Expr parse_expr(Parser* p, GenMode mode);

void token_error(Context* ctx, u32 start_index, u32 end_index, const char* msg);

#endif // PARSE_H
