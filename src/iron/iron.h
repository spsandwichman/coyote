#ifndef IRON_H
#define IRON_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdalign.h>

// feel free to define your own allocator.
#ifndef FE_DONT_USE_STD_HEAP
    #define fe_malloc(sz)       malloc(sz)
    #define fe_zalloc(sz)       memset(malloc(sz), 0, sz)
    #define fe_realloc(ptr, sz) realloc(ptr, sz)
#else
    // define these yourself!
    void* fe_malloc(size_t size);
    void* fe_zalloc(size_t size);
    void* fe_realloc(void* ptr, size_t size);
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;

typedef intptr_t  isize;
typedef uintptr_t usize;
#define USIZE_MAX UINTPTR_MAX
#define ISIZE_MAX INTPTR_MAX

typedef double   f64;
typedef float    f32;
typedef _Float16 f16;

#ifndef for_n
    #define for_n(iterator, start, end) for (usize iterator = (start); iterator < (end); ++iterator)
#endif

// crash at runtime with a stack trace (if available)
void fe_runtime_crash(const char* error, ...);

// initialize signal handler that calls fe_runtime_crash
// in the event of a bad signal
void fe_init_signal_handler();

typedef struct FeInst FeInst;
typedef struct FeBlock FeBlock;
typedef struct FeSymbol FeSymbol;
typedef struct FeFunction FeFunction;
typedef struct FeFunctionPrototype FeFunctionPrototype;
typedef struct FeModule FeModule;

typedef struct FeFunction {
    FeBlock* entry_block;
} FeFunction;

typedef struct FeFunctionPrototype {
    
} FeFunctionPrototype;

enum FeArchEnum {
    FE_ARCH_NONE = 0,
    FE_ARCH_XR,
};

typedef struct FeBlock {
    FeInst* bookend;
    FeFunction* funk;
    FeBlock* list_next;
    FeBlock* list_prev;
} FeBlock;

#define for_inst(inst, blockptr) \
    for (FeInst* inst = (blockptr)->bookend->next; inst->kind != FE_BOOKEND; inst = inst->next)

#define for_inst_reverse(inst, blockptr) \
    for (FeInst* inst = (blockptr)->bookend->prev; inst->kind != FE_BOOKEND; inst = inst->prev)


enum FeTyEnum {
    FE_TY_VOID,

    FE_TY_I8,
    FE_TY_I16,
    FE_TY_I32,
    FE_TY_I64,

    FE_TY_F16,
    FE_TY_F32,
    FE_TY_F64,

    FE_TY_V128,
    FE_TY_V256,
    FE_TY_V512,

    // member types dependent on source inst
    FE_TY_TUPLE,
};

enum FeInstKindEnum {
    // Bookend
    FE_BOOKEND = 1,

    // Proj
    FE_PROJ,

    // Binop
    FE_IADD,
    FE_ISUB,
    FE_IMUL, FE_UMUL, 
    FE_IDIV, FE_UDIV,
    FE_IREM, FE_UREM,
    FE_AND,
    FE_OR,
    FE_XOR,
    FE_SHL,
    FE_LSR, FE_ASR,
    FE_ILT, FE_ULT,
    FE_ILE, FE_ULE,
    FE_EQ,
    FE_NE,

    FE_FADD,
    FE_FSUB,
    FE_FMUL,
    FE_FDIV,
    FE_FREM,
    
    // Unop
    FE_MOV,
    FE_NOT,
    FE_NEG,
    FE_TRUNCATE, // integer downcast
    FE_SIGNEXT,  // signed integer upcast
    FE_ZEROEXT,  // unsigned integer upcast
    FE_INTTOFLOAT,  // integer to float
    FE_FLOATTOINT,  // float to integer

    // Load
    FE_LOAD,
    FE_LOAD_UNIQUE,
    FE_LOAD_VOLATILE,

    // Store
    FE_STORE,
    FE_STORE_UNIQUE,
    FE_STORE_VOLATILE,

    // (none)
    FE_CASCADE_VOLATILE,
    FE_CASCADE_UNIQUE,

    // Branch
    FE_BRANCH,

    // Jump
    FE_JUMP,

    _FE_BASE_INST_MAX,

    // architecture-specific nodes.
    _FE_XR_INST_START = FE_ARCH_XR * 256,
    _FE_XR_INST_END = _FE_XR_INST_START + 256,

    _FE_INST_MAX,
};

typedef struct FeInst {
    u16 kind;
    u8 ty;
    u32 flags; // for various things
    FeInst* prev;
    FeInst* next;

    usize extra[];
} FeInst;

#define fe_extra(instptr) ((void*)&(instptr)->extra[0])
#define fe_extra_T(instptr, T) ((T*)&(instptr)->extra[0])
#define fe_from_extra(extraptr) ((FeInst*)((usize)extraptr - offsetof(FeInst, extra)))

typedef struct {
    FeBlock* block;
} FeInstBookend;

typedef struct {
    FeInst* val;
    usize idx;
} FeInstProj;

typedef struct {
    FeInst* un;
} FeInstUnop;

typedef struct {
    FeInst* lhs;
    FeInst* rhs;
} FeInstBinop;

typedef struct {
    FeInst* ptr;
    // something something alignment something
} FeInstLoad;

typedef struct {
    FeInst* ptr;
    FeInst* val;
} FeInstStore;

typedef struct {
    FeInst* cond;
    FeBlock* if_true;
    FeBlock* if_false;
} FeInstBranch;

typedef struct {
    FeBlock* to;
} FeInstJump;

// check this assumption with some sort
// with a runtime init function
#define FE_INST_EXTRA_MAX_SIZE sizeof(FeInstBranch)

#define FE_IPOOL_FREE_SPACES_LEN FE_INST_EXTRA_MAX_SIZE / sizeof(usize) + 1
typedef struct FeInstPoolChunk FeInstPoolChunk;
typedef struct FeInstPoolFreeSpace FeInstPoolFreeSpace;
typedef struct {
    FeInstPoolChunk* top;
    FeInstPoolFreeSpace* free_spaces[FE_IPOOL_FREE_SPACES_LEN];
} FeInstPool;

void fe_ipool_init(FeInstPool* pool);
FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size);
void fe_ipool_free(FeInstPool* pool, FeInst* inst);
void fe_ipool_destroy(FeInstPool* pool);

usize fe_inst_extra_size(u8 kind);
usize fe_inst_extra_size_unsafe(u8 kind);

#endif // IRON_H