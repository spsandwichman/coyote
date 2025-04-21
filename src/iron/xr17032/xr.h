#ifndef FE_XR_H
#define FE_XR_H

#include "iron/iron.h"

enum {
    // XrRegImm16
    XR_ADDI = _FE_XR_INST_BEGIN, 
                    // out = reg + uimm16
    XR_SUBI,        // out = reg - uimm16
    XR_SLTI,        // out = reg < uimm16
    XR_SLTI_SIGNED, // out = signed reg < simm16
    XR_ANDI,        // out = reg & uimm16
    XR_XORI,        // out = reg ^ uimm16
    XR_ORI,         // out = reg | uimm16
    XR_LUI,         // out = reg | (uimm16 << 16)
    XR_MOV,         // out = reg

    XR_LOAD8_IMM,   // out = mem[reg + uimm16]
    XR_LOAD16_IMM,  // out = mem[reg + uimm16]
    XR_LOAD32_IMM,  // out = mem[reg + uimm16]

    // XrRegRegImm16
    XR_STORE8_IMM,  // mem[r1 + uimm16] = r2
    XR_STORE16_IMM, // mem[r1 + uimm16] = r2
    XR_STORE32_IMM, // mem[r1 + uimm16] = r2

    // XrRegImm16Imm5
    XR_STORE8_CONST,  // mem[reg + uimm16] = simm5
    XR_STORE16_CONST, // mem[reg + uimm16] = simm5
    XR_STORE32_CONST, // mem[reg + uimm16] = simm5

    // XrRegReg
    XR_SHIFT,      // out = r1 SHIFT r2
    XR_ADD,        // out = r1 + r2 SHIFT uimm5
    XR_SUB,        // out = r1 - r2 SHIFT uimm5
    XR_SLT,        // out = r1 < r2 SHIFT uimm5
    XR_SLT_SIGNED, // out = signed r1 < r2 SHIFT uimm5
    XR_AND,        // out = r1 < r2 SHIFT uimm5
    XR_XOR,        // out = r1 < r2 SHIFT uimm5
    XR_OR,         // out = r1 < r2 SHIFT uimm5
    XR_NOR,        // out = r1 < r2 SHIFT uimm5
    XR_MUL,        // out = r1 * r2
    XR_DIV,        // out = r1 / r2
    XR_DIV_SIGNED, // out = signed r1 / r2
    XR_MOD,        // out = r1 % r2
    XR_LOAD8_REG,  // out = mem[r1 + r2 SHIFT uimm5]
    XR_LOAD16_REG, // out = mem[r1 + r2 SHIFT uimm5]
    XR_LOAD32_REG, // out = mem[r1 + r2 SHIFT uimm5]

    // XrRegRegReg
    XR_STORE8_REG,  // mem[r1 + r2 SHIFT uimm5] = r3
    XR_STORE16_REG, // mem[r1 + r2 SHIFT uimm5] = r3
    XR_STORE32_REG, // mem[r1 + r2 SHIFT uimm5] = r3

    // XrRegBranch
    XR_BEQ,
    XR_BNE,
    XR_BLT,
    XR_BGT,
    XR_BLE,
    XR_BGE,

    // void
    XR_RET,
};

// #define fe_kind_is_xr(kind) (_FE_XR_INST_BEGIN <= (kind) && (kind) <= _FE_XR_INST_END)

typedef struct {
    FeInst* reg;
    u16 imm16;
} XrRegImm16;

typedef struct {
    FeInst* r1;
    FeInst* r2;
    u16 imm16;
} XrRegRegImm16;

typedef struct {
    FeInst* reg;
    u16 imm16;
    u8 imm5;
} XrRegImm16Imm5;

typedef u8 XrShiftKind;
enum XrShiftKindEnum {
    XR_SHIFT_LSH,
    XR_SHIFT_RSH,
    XR_SHIFT_ASH,
    XR_SHIFT_ROR,
};

typedef struct {
    FeInst* r1;
    FeInst* r2;

    u8 imm5;
    XrShiftKind shift_kind;
} XrRegReg;

typedef struct {
    FeInst* r1;
    FeInst* r2;
    FeInst* r3;

    u8 imm5;
    XrShiftKind shift_kind;
} XrRegRegReg;

typedef struct {
    FeInst* reg;
    FeBlock* dest;
    FeBlock* _else; // gets emitted as a secondary jump IF NECESSARY
} XrRegBranch;

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

extern u16 xr_regclass_lens[];
extern u8 xr_size_table[];
extern FeTrait xr_trait_table[];

char* xr_inst_name(FeInstKind kind, bool ir);
char* xr_reg_name(u8 regclass, u16 real);
FeRegStatus xr_reg_status(u8 cconv, u8 regclass, u16 real);

void xr_print_args(FeFunction* f, FeDataBuffer* db, FeInst* inst);
FeInst** xr_list_inputs(FeInst* inst, usize* len_out);
FeBlock** xr_term_list_targets(FeInst* term, usize* len_out);

FeInstChain xr_isel(FeFunction* f, FeBlock* block, FeInst* inst);
void xr_pre_regalloc_opt(FeFunction* f);
void xr_final_touchups(FeFunction* f);

void xr_emit_assembly(FeFunction* f, FeDataBuffer* db);

FeRegclass xr_choose_regclass(FeInstKind kind, FeTy ty);

#endif