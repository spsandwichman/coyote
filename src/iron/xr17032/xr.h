#ifndef FE_XR_H
#define FE_XR_H

#include "iron/iron.h"

enum {
    // FeXrRegImm16

    FE_XR_ADDI = _FE_XR_INST_BEGIN, // out = rb + uimm16
    FE_XR_LUI, // out = rb | (uimm16 << 16)
    FE_XR_SUBI, // out = rb - uimm16
    FE_XR_SLTI, // out = rb < uimm16

    // FeXrRegReg
    FE_XR_ADD,  // out = rb + rc
    FE_XR_SUB,  // out = rb - rc
    FE_XR_MUL,  // out = rb * rc

    // FeXrRegBranch
    FE_XR_BEQ,
    FE_XR_BNE,
    FE_XR_BLT,
    FE_XR_BGT,
    FE_XR_BLE,
    FE_XR_BGE,

    // void
    FE_XR_RET,
};

// #define fe_kind_is_xr(kind) (_FE_XR_INST_BEGIN <= (kind) && (kind) <= _FE_XR_INST_END)

typedef struct {
    FeInst* reg;
    u16 num;
} FeXrRegImm16;

typedef struct {
    FeInst* lhs;
    FeInst* rhs;
} FeXrRegReg;

typedef struct {
    FeInst* reg;
    FeBlock* dest;
    FeBlock* _else; // gets emitted as a secondary jump IF NECESSARY
} FeXrRegBranch;

enum {
    XR_REGCLASS_REG = 1,
};

enum {
    XR_REG_ZERO,

    // temporary registers (jkl: call-clobbered)
    XR_REG_T0, XR_REG_T1, XR_REG_T2, XR_REG_T3, XR_REG_T4, XR_REG_T5,

    // arguments and returns (jkl: call-clobbered)
    XR_REG_A0, XR_REG_A1, XR_REG_A2, XR_REG_A3,

    // locals (jkl: call-preserved)
    XR_REG_S0, XR_REG_S1, XR_REG_S2, XR_REG_S3, XR_REG_S4, XR_REG_S5, 
    XR_REG_S6, XR_REG_S7, XR_REG_S8, XR_REG_S9, XR_REG_S10, XR_REG_S11, 
    XR_REG_S12, XR_REG_S13, XR_REG_S14, XR_REG_S15, XR_REG_S16, XR_REG_S17,

    // TLS pointer (unused as far as i know? i wont touch it)
    XR_REG_TP,
    // stack pointer
    XR_REG_SP,
    // link register
    XR_REG_LR,

    XR_REG__COUNT,
};

extern u8 xr_size_table[];
extern FeTrait xr_trait_table[];

char* xr_inst_name(FeInstKind kind, bool ir);
char* xr_reg_name(u8 regclass, u16 real);
FeRegStatus xr_reg_status(u8 cconv, u8 regclass, u16 real);

void xr_print_args(FeFunction* f, FeDataBuffer* db, FeInst* inst);
FeInst** xr_list_inputs(FeInst* inst, usize* len_out);
FeBlock** xr_term_list_targets(FeInst* term, usize* len_out);

FeInstChain xr_isel(FeFunction* f, FeBlock* block, FeInst* inst);
void xr_opt(FeFunction* f);

void xr_emit_assembly(FeFunction* f, FeDataBuffer* db);

FeRegclass xr_choose_regclass(FeInstKind kind, FeTy ty);

#endif