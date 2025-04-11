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

void fe_runtime_crash(const char* error, ...);
void fe_init_signal_handler();

typedef struct FeInst FeInst;
typedef struct FeBlock FeBlock;
typedef struct FeFunction FeFunction;
typedef struct FeModule FeModule;

// typedef struct FeTargetDef {

// } FeTargetDef;

typedef struct FeBlock {
    FeInst* bookend;
} FeBlock;

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
    
    // Unop
    FE_MOV,
    FE_NOT,
    FE_NEG,

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

    
    _FE_BASE_INST_MAX,

    _FE_XR_INST_START,
    _FE_XR_INST_END = _FE_XR_INST_START + 256,

    _FE_INST_MAX,
};

typedef struct FeInst {
    u16 kind;
    u8 ty;
    FeInst* prev;
    FeInst* next;

    usize extra[];
} FeInst;

#define fe_extra(instptr) ((void*)&(instptr)->extra)
#define fe_extra_T(instptr, T) ((T*)&(instptr)->extra)
#define fe_from_extra(extraptr) ((FeInst*)((usize)extraptr - offsetof(FeInst, extra)))

typedef struct {
    FeBlock* block;
} FeInstBookend;

typedef struct {
    FeInst* src;
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
    // something something alignment
} FeInstLoad;

typedef struct {
    FeInst* ptr;
    FeInst* val;
} FeInstStore;

// check this assumption with some sort
// with a runtime init function
#define FE_INST_EXTRA_MAX_SIZE sizeof(FeInstBinop)

#define FE_IPOOL_FREE_SPACES_LEN FE_INST_EXTRA_MAX_SIZE / sizeof(usize) + 1
typedef struct FeInstPoolChunk FeInstPoolChunk;
typedef struct FeInstPoolFreeSpace FeInstPoolFreeSpace;
typedef struct {
    FeInstPoolChunk* front;
    FeInstPoolFreeSpace* free_spaces[FE_IPOOL_FREE_SPACES_LEN];
} FeInstPool;

FeInstPool fe_ipool_new();
FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size);
void fe_ipool_free(FeInstPool* pool, FeInst* inst);

usize fe_inst_extra_size(u8 kind);
usize fe_inst_extra_size_unsafe(u8 kind);

#endif // IRON_H