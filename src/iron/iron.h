#ifndef IRON_H
#define IRON_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if __STDC_VERSION__ <= 201710L 
    #error "Iron is a C23 library!"
#endif

// feel free to make iron use your own heap-like allocator.
#ifndef FE_CUSTOM_ALLOCATOR
    #define fe_malloc  malloc
    #define fe_realloc realloc
    #define fe_free    free
#else
    void* fe_malloc(usize size);
    void* fe_realloc(void* ptr, usize size);
    void fe_free(void* ptr, usize size);
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

// simple ++n loop
#ifndef for_n
    #define for_n(iterator, start, end) for (usize iterator = (start); iterator < (end); ++iterator)
#endif

#define FE_TY_VEC(vecty, elemty) (vecty + elemty)
#define FE_TY_VEC_ELEM(ty) (ty & 0b1111)
#define FE_TY_VEC_SIZETY(ty) (ty & 0b110000)
#define FE_TY_IS_VEC(ty) (ty > FE_TY_V128)

typedef enum: u8 {
    FE_TY_VOID,

    FE_TY_BOOL,

    FE_TY_I8,
    FE_TY_I16,
    FE_TY_I32,
    FE_TY_I64,

    FE_TY_F16,
    FE_TY_F32,
    FE_TY_F64,

    // member types dependent on source inst
    FE_TY_TUPLE,

    FE_TY_V128 = 0b010000, // elem type fits in lower 4 bits
    FE_TY_V256 = 0b100000,
    FE_TY_V512 = 0b110000,

    FE_TY_I8x16 = FE_TY_VEC(FE_TY_V128, FE_TY_I8),
    FE_TY_I16x8 = FE_TY_VEC(FE_TY_V128, FE_TY_I16),
    FE_TY_I32x4 = FE_TY_VEC(FE_TY_V128, FE_TY_I32),
    FE_TY_I64x2 = FE_TY_VEC(FE_TY_V128, FE_TY_I64),
    FE_TY_F16x8 = FE_TY_VEC(FE_TY_V128, FE_TY_F16),
    FE_TY_F32x4 = FE_TY_VEC(FE_TY_V128, FE_TY_F32),
    FE_TY_F64x2 = FE_TY_VEC(FE_TY_V128, FE_TY_F64),

    FE_TY_I8x32  = FE_TY_VEC(FE_TY_V256, FE_TY_I8),
    FE_TY_I16x16 = FE_TY_VEC(FE_TY_V256, FE_TY_I16),
    FE_TY_I32x8  = FE_TY_VEC(FE_TY_V256, FE_TY_I32),
    FE_TY_I64x4  = FE_TY_VEC(FE_TY_V256, FE_TY_I64),
    FE_TY_F16x16 = FE_TY_VEC(FE_TY_V256, FE_TY_F16),
    FE_TY_F32x8  = FE_TY_VEC(FE_TY_V256, FE_TY_F32),
    FE_TY_F64x4  = FE_TY_VEC(FE_TY_V256, FE_TY_F64),
    
    FE_TY_I8x64  = FE_TY_VEC(FE_TY_V512, FE_TY_I8),
    FE_TY_I16x32 = FE_TY_VEC(FE_TY_V512, FE_TY_I16),
    FE_TY_I32x16 = FE_TY_VEC(FE_TY_V512, FE_TY_I32),
    FE_TY_I64x8  = FE_TY_VEC(FE_TY_V512, FE_TY_I64),
    FE_TY_F16x32 = FE_TY_VEC(FE_TY_V512, FE_TY_F16),
    FE_TY_F32x16 = FE_TY_VEC(FE_TY_V512, FE_TY_F32),
    FE_TY_F64x8  = FE_TY_VEC(FE_TY_V512, FE_TY_F64),
} FeTy;

typedef enum: u8 {
    FE_ARCH_X86_64 = 1,

    // esoteric
    FE_ARCH_XR17032,
    FE_ARCH_FOX32,
    FE_ARCH_APHELION,
} FeArch;

typedef enum: u8 {
    FE_SYSTEM_FREESTANDING = 1,
} FeSystem;

typedef enum: u8 {
    // pass parameters however iron sees fit
    // likely picks based on target
    FE_CCONV_ANY = 0,

    // std linux c callconv
    FE_CCONV_SYSV,
    // std windows c callconv
    FE_CCONV_STDCALL,

    // language-specific/esoteric
    FE_CCONV_JACKAL, // hi will
} FeCallConv;

typedef struct FeInst FeInst;
typedef struct FeBlock FeBlock;
typedef struct FeSymbol FeSymbol;
typedef struct FeFunc FeFunc;
typedef struct FeFuncSig FeFuncSig;
typedef struct FeModule FeModule;
typedef struct FeTarget FeTarget;
typedef struct FeStackItem FeStackItem;
typedef struct FeInstPool FeInstPool;

typedef u32 FeVReg; // vreg index.
typedef struct FeVRegBuffer FeVRegBuffer;
typedef struct FeBlockLiveness FeBlockLiveness;

typedef enum: u8 {
    FE_BIND_LOCAL = 1,
    FE_BIND_GLOBAL,

    // shared libary stuff
    FE_BIND_SHARED_EXPORT,
    FE_BIND_SHARED_IMPORT,
} FeSymbolBinding;

typedef enum: u8 {
    FE_SYMKIND_FUNC = 1,
    FE_SYMKIND_DATA,
    FE_SYMKIND_EXTERN, // not defined in this unit
} FeSymbolKind;

typedef struct FeSymbol {
    const char* name;
    u16 name_len;
    FeSymbolKind kind; // what does this symbol refer to?
    FeSymbolBinding bind; // where is this symbol visible?

    union {
        FeFunc* func;
    };
} FeSymbol;

typedef struct FeFuncParam {
    FeTy ty;
} FeFuncParam;

// holds type/interface information about a function.
typedef struct FeFuncSig {
    FeCallConv cconv;
    u16 param_len;
    u16 return_len;
    FeFuncParam params[];
} FeFuncSig;

typedef struct FeFunc {
    FeFuncSig* sig;
    FeSymbol* sym;
    FeModule* mod;

    FeInstPool* ipool;
    FeVRegBuffer* vregs;

    FeInst** params; // gets length of params from signature

    FeBlock* entry_block;
    FeBlock* last_block;
    
    FeFunc* list_next;
    FeFunc* list_prev;

    FeStackItem* stack_top; // most-positive offset from stack pointer
    FeStackItem* stack_bottom;
} FeFunc;

typedef struct FeModule {
    const FeTarget* target;
    struct {
        FeFunc* first;
        FeFunc* last;
    } funcs;
} FeModule;

typedef struct FeCFGNode FeCFGNode;
typedef struct FeCFGNode {
    FeBlock* block;
    u16 out_len;
    u16 in_len;
    u16 rev_post;

    FeCFGNode** ins;
} FeCFGNode;

#define fe_cfgn_in(cfgn, i) ((cfgn)->ins[i])
#define fe_cfgn_out(cfgn, i) ((cfgn)->ins[i + (cfgn)->in_len])

typedef struct FeBlock {
    FeFunc* func;
    u32 flags;
    FeInst* bookend;

    FeBlock* list_next;
    FeBlock* list_prev;

    // dominance/cfg info
    FeCFGNode* cfg_node;
    // liveness info
    FeBlockLiveness* live;
} FeBlock;

#define for_funcs(f, modptr) \
    for (FeFunc* f = (modptr)->funcs.first; f != nullptr; f = f->list_next)

#define for_blocks(block, funcptr) \
    for (FeBlock* block = (funcptr)->entry_block; block != nullptr; block = block->list_next)

#define for_inst(inst, blockptr) \
    for (FeInst* inst = (blockptr)->bookend->next, *_next_ = inst->next; inst->kind != FE_BOOKEND; inst = _next_, _next_ = _next_->next)

#define for_inst_reverse(inst, blockptr) \
    for (FeInst* inst = (blockptr)->bookend->prev, *_prev_ = inst->prev; inst->kind != FE_BOOKEND; inst = _prev_, _prev_ = _prev_->prev)

typedef u16 FeInstKind;
typedef enum: FeInstKind {
    // Bookend
    FE_BOOKEND = 1,

    // Param
    FE_PARAM,

    // Proj
    FE_PROJ,
    FE_MACH_PROJ, // mostly for hardcoding register clobbers

    // Const
    FE_CONST,

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
    FE_USR, FE_ISR,
    
    FE_ILT, FE_ULT,
    FE_ILE, FE_ULE,
    FE_EQ,
    
    FE_FADD,
    FE_FSUB,
    FE_FMUL,
    FE_FDIV,
    FE_FREM,
    
    // Unop
    FE_MOV,
    FE_MACH_MOV, // mostly for hardcoding register clobbers
    FE_UPSILON,
    
    FE_NOT,
    FE_NEG,
    FE_TRUNC, // integer downcast
    FE_SIGNEXT,  // signed integer upcast
    FE_ZEROEXT,  // unsigned integer upcast
    FE_BITCAST,
    FE_I2F, // integer to float
    FE_F2I, // float to integer

    // Load
    FE_LOAD,
    FE_LOAD_UNIQUE,
    FE_LOAD_VOLATILE,

    // Store
    FE_STORE,
    FE_STORE_UNIQUE,
    FE_STORE_VOLATILE,

    // (void)
    FE_CASCADE_UNIQUE,
    FE_CASCADE_VOLATILE,
    FE_MACH_REG,

    // FeMachStackSpill
    FE_MACH_STACK_SPILL,
    // FeMachStackReload
    FE_MACH_STACK_RELOAD,

    // Branch
    FE_BRANCH,

    // Jump
    FE_JUMP,

    // Return
    FE_RETURN,

    // Phi
    FE_PHI,

    // CallDirect
    FE_CALL_DIRECT,
    // CallIndirect
    FE_CALL_INDIRECT,

    _FE_BASE_INST_END,

    _FE_XR_INST_BEGIN = FE_ARCH_XR17032 * 256,
    _FE_XR_INST_END = _FE_XR_INST_BEGIN + 256,

    _FE_INST_END,
} FeInstKindGeneric;

typedef struct FeInst {
    FeInstKind kind;
    FeTy ty;
    // expect this to be overwritten by different passes for different things
    u32 flags;
    u32 use_count;
    FeVReg vr_out;

    // CIRCULAR
    FeInst* prev;
    FeInst* next;

    usize extra[];
} FeInst;

typedef struct {
    FeBlock* block;
} FeInstBookend;

typedef struct {
    FeInst* val;
    usize idx;
} FeInstProj;

typedef struct {
    usize index;
} FeInstParam;

typedef struct {
    u64 val;
} FeInstConst;

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
    // TODO pack this. doesnt make sense to waste a word's worth of bytes on this.
    FeTy store_ty;
} FeInstStore;

typedef struct {
    FeInst* cond;
    FeBlock* if_true;
    FeBlock* if_false;
} FeInstBranch;

typedef struct {
    FeBlock* to;
} FeInstJump;

typedef struct {
    u16 len;
    u16 cap;
    FeInst** vals;
    FeBlock** blocks;
} FeInstPhi;

typedef struct {
    u16 len;
    u16 cap; // if cap == 0, use single.
    union {
        FeInst*  single;
        FeInst** multi;
    };
} FeInstReturn;

typedef struct {
    u16 len;
    u16 cap; // if cap == 0, use single.
    union {
        FeInst*  single_arg;
        FeInst** multi_arg;
    };

    FeFunc* to_call;
} FeInstCallDirect;

typedef struct {
    u16 len;
    u16 cap; // if cap == 0, use single.
    union {
        FeInst*  single_arg;
        FeInst** multi_arg;
    };

    FeInst* to_call;
    FeFuncSig* sig;
} FeInstCallIndirect;

typedef struct {
    FeInst* val;
    FeStackItem* item;
} FeMachStackSpill;

typedef struct {
    FeStackItem* item;
} FeMachStackReload;

#define FE_STACK_OFFSET_UNDEF UINT32_MAX

typedef struct FeStackItem {
    FeStackItem* next; // points to top
    FeStackItem* prev; // points to bottom

    u16 size;
    u16 align;

    u32 _offset;
} FeStackItem;

FeStackItem* fe_stack_item_new(u16 size, u16 align);
FeStackItem* fe_stack_append_bottom(FeFunc* f, FeStackItem* item);
FeStackItem* fe_stack_append_top(FeFunc* f, FeStackItem* item);
u32 fe_stack_calculate_size(FeFunc* f);

typedef enum : u16 {
    // x op y == y op x
    FE_TRAIT_COMMUTATIVE      = 1u << 0,
    // (x op y) op z == x op (y op z)
    FE_TRAIT_ASSOCIATIVE      = 1u << 1,
    // (x op y) op z == x op (y op z) trust me vro 💀🙏🥀
    FE_TRAIT_FAST_ASSOCIATIVE = 1u << 2,
    // cannot be simply removed if result is not used
    FE_TRAIT_VOLATILE         = 1u << 3,
    // can only terminate a basic block
    FE_TRAIT_TERMINATOR       = 1u << 4,
    // output type = first input type
    FE_TRAIT_SAME_IN_OUT_TY   = 1u << 5,
    // all input types must be the same type  
    FE_TRAIT_SAME_INPUT_TYS   = 1u << 6,
    // all input types must be integers
    FE_TRAIT_INT_INPUT_TYS    = 1u << 7,
    // all input types must be floats
    FE_TRAIT_FLT_INPUT_TYS    = 1u << 8,
    // allow vector types
    FE_TRAIT_VEC_INPUT_TYS    = 1u << 9,
    // output type = bool
    FE_TRAIT_BOOL_OUT_TY      = 1u << 10,
    // reg->reg move; allocator should hint
    // the output and input to be the same
    FE_TRAIT_REG_MOV_HINT     = 1u << 11,
} FeTrait;

bool fe_inst_has_trait(FeInstKind kind, FeTrait trait);
void fe__load_trait_table(usize start_index, FeTrait* table, usize len);

usize fe_inst_extra_size(FeInstKind kind);
usize fe_inst_extra_size_unsafe(FeInstKind kind);
void fe__load_extra_size_table(usize start_index, u8* table, usize len);

FeModule* fe_module_new(FeArch arch, FeSystem system);
void fe_module_destroy(FeModule* mod);

FeSymbol* fe_symbol_new(FeModule* mod, const char* name, u16 len, FeSymbolBinding bind);
void fe_symbol_destroy(FeSymbol* sym);

FeFuncSig* fe_funcsig_new(FeCallConv cconv, u16 param_len, u16 return_len);
FeFuncParam* fe_funcsig_param(FeFuncSig* sig, u16 index);
FeFuncParam* fe_funcsig_return(FeFuncSig* sig, u16 index);
void fe_funcsig_destroy(FeFuncSig* sig);

FeBlock* fe_block_new(FeFunc* f);
void fe_block_destroy(FeBlock* block);

FeFunc* fe_func_new(
    FeModule* mod,
    FeSymbol* sym,
    FeFuncSig* sig,
    FeInstPool* ipool,
    FeVRegBuffer* vregs);
void fe_func_destroy(FeFunc* f);
FeInst* fe_func_param(FeFunc* f, u16 index);

FeInst* fe_insert_before(FeInst* point, FeInst* i);
FeInst* fe_insert_after(FeInst* point, FeInst* i);
#define fe_append_begin(block, inst) fe_insert_after((block)->bookend, inst)
#define fe_append_end(block, inst) fe_insert_before((block)->bookend, inst)
FeInst* fe_inst_remove_pos(FeInst* inst);
void fe_inst_replace_pos(FeInst* from, FeInst* to);

// a "chain" is a free-floating (not attached to a block)
// list of instructions.
typedef struct FeInstChain {
    FeInst* begin;
    FeInst* end;
} FeInstChain;

FeInstChain fe_chain_new(FeInst* initial);
FeInstChain fe_chain_append_end(FeInstChain chain, FeInst* i);
FeInstChain fe_chain_append_begin(FeInstChain chain, FeInst* i);
void fe_insert_chain_before(FeInst* point, FeInstChain chain);
void fe_insert_chain_after(FeInst* point, FeInstChain chain);
void fe_chain_replace_pos(FeInst* from, FeInstChain to);
void fe_chain_destroy(FeFunc* f, FeInstChain chain);

void fe_inst_free(FeFunc* f, FeInst* inst);
void fe_inst_update_uses(FeFunc* f);
FeInst** fe_inst_list_inputs(const FeTarget* t, FeInst* inst, usize* len_out);
FeBlock** fe_inst_term_list_targets(const FeTarget* t, FeInst* term, usize* len_out);

FeTy fe_proj_ty(FeInst* tuple, usize index);

FeInst* fe_inst_const(FeFunc* f, FeTy ty, u64 val);
FeInst* fe_inst_unop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* val);
FeInst* fe_inst_binop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* lhs, FeInst* rhs);
FeInst* fe_inst_bare(FeFunc* f, FeTy ty, FeInstKind kind);
FeInst* fe_inst_call_direct(FeFunc* f, FeFunc* to_call);
FeInst* fe_inst_call_indirect(FeFunc* f, FeInst* to_call, FeFuncSig* sig);
FeInst* fe_call_arg(FeInst* call, usize index);
void fe_call_set_arg(FeInst* call, usize index, FeInst* arg);
FeInst* fe_inst_return(FeFunc* f);
FeInst* fe_return_arg(FeInst* ret, usize index);
void fe_return_set_arg(FeInst* ret, usize index, FeInst* arg);

FeInst* fe_inst_branch(FeFunc* f, FeInst* cond, FeBlock* if_true, FeBlock* if_false);
FeInst* fe_inst_jump(FeFunc* f, FeBlock* to);

FeInst* fe_inst_phi(FeFunc* f, FeTy ty, u16 num_srcs);
FeInst* fe_phi_get_src_val(FeInst* inst, u16 index);
FeBlock* fe_phi_get_src_block(FeInst* inst, u16 index);
void fe_phi_set_src(FeInst* inst, u16 index, FeInst* val, FeBlock* block);
void fe_phi_append_src(FeInst* inst, FeInst* val, FeBlock* block);
void fe_phi_remove_src_unordered(FeInst* inst, u16 index);

#define fe_extra(instptr) ((void*)&(instptr)->extra[0])
#define fe_extra_T(instptr, T) ((T*)&(instptr)->extra[0])
#define fe_from_extra(extraptr) ((FeInst*)((usize)extraptr - offsetof(FeInst, extra)))

// check this assumption with some sort
// with a runtime init function
#define FE_INST_EXTRA_MAX_SIZE sizeof(FeInstCallIndirect)

#define FE_IPOOL_FREE_SPACES_LEN (FE_INST_EXTRA_MAX_SIZE / sizeof(usize) + 1)
typedef struct FeInstPoolChunk FeInstPoolChunk;
typedef struct FeInstPoolFreeSpace FeInstPoolFreeSpace;
typedef struct FeInstPool {
    FeInstPoolChunk* top;
    FeInstPoolFreeSpace* free_spaces[FE_IPOOL_FREE_SPACES_LEN];
} FeInstPool;

void fe_ipool_init(FeInstPool* pool);
FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size);
void fe_ipool_free(FeInstPool* pool, FeInst* inst);
usize fe_ipool_free_manual(FeInstPool* pool, FeInst* inst);
void fe_ipool_destroy(FeInstPool* pool);

// ----------------------------- passes ------------------------------

void fe_cfg_calculate(FeFunc* f);
void fe_cfg_destroy(FeFunc* f);

void fe_opt_tdce(FeFunc* f);
void fe_opt_algsimp(FeFunc* f);

// ------------------------------ utils ------------------------------

// like stringbuilder but epic
typedef struct FeDataBuffer {
    u8* at;
    usize len;
    usize cap;
} FeDataBuffer;

void fe_db_init(FeDataBuffer* buf, usize cap);
void fe_db_clone(FeDataBuffer* dst, FeDataBuffer* src);
void fe_db_destroy(FeDataBuffer* buf) ;

char* fe_db_clone_to_cstring(FeDataBuffer* buf);

// reserve bytes
void fe_db_reserve(FeDataBuffer* buf, usize more);

// reserve until buf->cap == new_cap
// does nothing when buf->cap > new_cap
void fe_db_reserve_until(FeDataBuffer* buf, usize new_cap);

// append data to end of buffer
usize fe_db_write(FeDataBuffer* buf, const void* ptr, usize len);
usize fe_db_writecstr(FeDataBuffer* buf, const char* s);
usize fe_db_write8(FeDataBuffer* buf, u8 data);
usize fe_db_write16(FeDataBuffer* buf, u16 data);
usize fe_db_write32(FeDataBuffer* buf, u32 data);
usize fe_db_write64(FeDataBuffer* buf, u64 data);
usize fe_db_writef(FeDataBuffer* buf, const char* fmt, ...);

void fe_print_func(FeDataBuffer* db, FeFunc* f);
void fe__print_block(FeDataBuffer* db, FeFunc* f, FeBlock* ref);
void fe__print_ref(FeDataBuffer* db, FeFunc* f, FeInst* ref);

// crash at runtime with a stack trace (if available)
[[noreturn]] void fe_runtime_crash(const char* error, ...);

// initialize signal handler that calls fe_runtime_crash
// in the event of a bad signal
void fe_init_signal_handler();

// ----------------------------- codegen -----------------------------

#define FE_ISEL_GENERATED 0
#define FE_VREG_REAL_UNASSIGNED UINT16_MAX
#define FE_VREG_NONE UINT32_MAX

typedef struct FeBlockLiveness {
    FeBlock* block;

    u16 in_len;
    u16 in_cap;
    
    u16 out_len;
    u16 out_cap;

    FeVReg* in;
    FeVReg* out;
} FeBlockLiveness;

typedef struct FeVirtualReg {
    u8 class;
    u16 real;
    bool is_phi_out;
    FeVReg hint;

    FeInst* def;
    FeBlock* def_block;
} FeVirtualReg;

typedef struct FeVRegBuffer {
    FeVirtualReg* at;
    u32 len;
    u32 cap;
} FeVRegBuffer;

typedef enum : u8 {
    FE_REG_CALL_CLOBBERED,
    FE_REG_CALL_PRESERVED,
    FE_REG_UNUSABLE,
} FeRegStatus;

typedef u8 FeRegclass;
#define FE_REGCLASS_NONE 0

typedef struct FeTarget {
    FeArch arch;
    FeSystem system;

    const char* (*inst_name)(FeInstKind kind, bool ir);
    FeInst** (*list_inputs)(FeInst* inst, usize* len_out);
    FeBlock** (*list_targets)(FeInst* term, usize* len_out);

    FeInstChain (*isel)(FeFunc* f, FeBlock* block, FeInst* inst);
    void (*pre_regalloc_opt)(FeFunc* f);
    void (*final_touchups)(FeFunc* f);

    FeRegclass (*choose_regclass)(FeInstKind kind, FeTy ty);
    const char* (*reg_name)(u8 regclass, u16 real);
    FeRegStatus (*reg_status)(u8 cconv, u8 regclass, u16 real);
    
    u8 max_regclass;
    const u16* regclass_lens;

    u64 stack_pointer_align;

    void (*ir_print_args)(FeDataBuffer* db, FeFunc* f, FeInst* inst);
    void (*emit_asm)(FeDataBuffer* db, FeFunc* f);
} FeTarget;

const FeTarget* fe_make_target(FeArch arch, FeSystem system);

void fe_regalloc_linear_scan(FeFunc* f);

void fe_vrbuf_init(FeVRegBuffer* buf, usize cap);
void fe_vrbuf_clear(FeVRegBuffer* buf);
void fe_vrbuf_destroy(FeVRegBuffer* buf);

FeVReg fe_vreg_new(FeVRegBuffer* buf, FeInst* def, FeBlock* def_block, u8 class);
FeVirtualReg* fe_vreg(FeVRegBuffer* buf, FeVReg vr);

void fe_codegen(FeFunc* f);
void fe_emit_asm(FeDataBuffer* db, FeFunc* f);

#endif