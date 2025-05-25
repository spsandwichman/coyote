#ifndef PARSE_H
#define PARSE_H

#include "coyote.h"
#include "lex.h"

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
    TY_ARRAY,
    TY_STRUCT,
    TY_STRUCT_PACKED,
    TY_UNION,
    TY_ENUM,
    TY_FNPTR,
    TY_FN,

    TY_ALIAS_INCOMPLETE,
    TY_ALIAS,
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
    u8 kind;
    TyIndex to;
    u32 len;
} TyArray;

typedef struct {
    u8 kind;
    TyIndex aliasing;
} TyAlias;

typedef struct {
    TyIndex type;
    u16 offset;
    CompactString name;
} Ty_RecordMember;

#define RECORD_MAX_FIELDS UINT8_MAX
typedef struct {
    u8 kind;
    u8 len; // should never be a limitation O.O
    u16 size;
    u8 align;
    CompactString name;
    Ty_RecordMember member[];
} TyRecord;

void ty_init();

// ------------------- PARSE/SEMA ------------------- 

void token_error(Parser* ctx, ReportKind kind, u32 start_index, u32 end_index, const char* msg);

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum : u8 {
    STORAGE_PUBLIC,
    STORAGE_PRIVATE,
    STORAGE_EXPORT,
    STORAGE_EXTERN,
} StorageKind;

typedef enum : u8 {
    ENTKIND_VAR,
    ENTKIND_FN,
    ENTKIND_TYPE,
} EntityKind;

typedef struct {
    CompactString name;

    EntityKind kind;
    StorageKind storage;
    TyIndex ty;

    Stmt* decl;
} Entity;

typedef enum : u8 {
    
    STMT_EXPR,

    STMT_VAR_DECL,

    STMT_ASSIGN,
    STMT_ASSIGN_ADD,
    STMT_ASSIGN_SUB,
    STMT_ASSIGN_MUL,
    STMT_ASSIGN_DIV,
    STMT_ASSIGN_MOD,
    STMT_ASSIGN_AND,
    STMT_ASSIGN_OR,
    STMT_ASSIGN_XOR,
    STMT_ASSIGN_LSH,
    STMT_ASSIGN_RSH,

    STMT_BARRIER,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_LEAVE,
    STMT_RETURN,

    STMT_IF,

    STMT_WHILE,

    STMT_LABEL,
    STMT_GOTO,
} StmtKind;

typedef struct StmtList {
    u32 len; 
    Stmt* stmts;
} StmtList;

typedef struct Stmt {
    StmtKind kind;
    u32 token_index;
    
    union { // allocated in the arena as-needed
        usize extra[0];
        
        Expr* expr;

        struct {
            Entity* var;
            Expr* expr;
        } var_decl;

        struct {
            Expr* lhs;
            Expr* rhs;
        } assign;

        struct {
            Stmt* loop;
        } break_continue;

        struct {
            Expr* cond;
            StmtList block;
            Stmt* else_;
        } if_;

        StmtList block;

        struct {
            Expr* cond;
            StmtList block;
        } while_;

        Entity* label;

        Entity* goto_;
    };
} Stmt;

typedef enum : u8 {
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_MOD,
    EXPR_AND,
    EXPR_OR,
    EXPR_XOR,
    EXPR_LSH,
    EXPR_RSH,

    EXPR_ADDROF,
    EXPR_NEG,
    EXPR_NOT,
    EXPR_BOOL_NOT,
    EXPR_SIZEOFVALUE,

    EXPR_CONTAINEROF,
    EXPR_CAST,

    EXPR_ENTITY,
    EXPR_STR_LITERAL,
    EXPR_LITERAL,

    // EXPR_TRUE,
    // EXPR_FALSE,
    // EXPR_NULLPTR,
    // EXPR_OFFSETOF,
    // EXPR_SIZEOF,
    // ^^^ these just get translated into int literals

    EXPR_SUBSCRIPT,     // foo[bar]
    EXPR_DEREF,         // foo^
    EXPR_DEREF_MEMBER,  // foo^.bar
    EXPR_MEMBER,        // foo.bar
    EXPR_CALL,          // foo(bar, baz)
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    TyIndex ty;
    u32 token_index;
    
    union { // allocated in the arena as-needed
        usize extra[0];

        u64 literal;
        CompactString lit_string;

        Entity* entity;

        Expr* unary;

        struct {
            Expr* lhs;
            Expr* rhs;
        } binary;

        struct {
            Expr* callee;
            Expr* args;
            u32 args_len;
        } call;
    };
} Expr;

Stmt* parse_stmt(Parser* p);
Expr* parse_expr(Parser* p);
TyIndex parse_type(Parser* p);

typedef struct Function {
    Entity* entity;
    TyIndex t;
    Stmt* decl;
    StmtList stmts;
} Function;

#endif // PARSE_H
