#ifndef FE_XR_H
#define FE_XR_H

#include "iron/iron.h"

enum {
    // FeXrRegImm16
    FE_XR_ADDI = _FE_XR_INST_BEGIN, // out = rb + uimm16
    FE_XR_SUBI, // out = rb - uimm16

    // FeInstBinop (lmao)
    FE_XR_ADD, // out = rb + rc
    FE_XR_SUB, // out = rb + rc
    FE_XR_MUL, // out = rb * rc

    // void
    FE_XR_RET,
};

#define fe_kind_is_xr(kind) (_FE_XR_INST_BEGIN <= (kind) && (kind) <= _FE_XR_INST_END)

typedef struct {
    FeInst* val;
    u16 num;
} FeXrRegImm16;

typedef struct {
    FeInst* lhs;
    FeInst* rhs;
} FeXrRegReg;

enum {
    _XR_REG_ZERO,

    // temporary registers (jkl: caller-saved)
    XR_REG_T0, XR_REG_T1, XR_REG_T2, XR_REG_T3, XR_REG_T4, XR_REG_T5,

    // arguments and returns (jkl: caller-saved)
    XR_REG_A0, XR_REG_A1, XR_REG_A2, XR_REG_A3, XR_REG_A4,

    // locals (jkl: callee-saved)
    XR_REG_S0, XR_REG_S1, XR_REG_S2, XR_REG_S3, XR_REG_S4, XR_REG_S5, XR_REG_S6, XR_REG_S7, XR_REG_S8,
    XR_REG_S9, XR_REG_S10, XR_REG_S11, XR_REG_S12, XR_REG_S13, XR_REG_S14, XR_REG_S15, XR_REG_S16, XR_REG_S17,

    // TLS pointer (unused as far as i know? i wont touch it)
    XR_REG_TP,
    // stack pointer
    XR_REG_SP,
    // link register
    XR_REG_LR,
};

usize fe_xr_extra_size_unsafe(FeInstKind kind);
char* fe_xr_inst_name(FeInstKind kind);
void fe_xr_print_args(FeDataBuffer* db, FeInst* inst);
FeInst** fe_xr_list_inputs(FeInst* inst, usize* len_out);

void fe_xr_isel(FeFunction* f);

#endif