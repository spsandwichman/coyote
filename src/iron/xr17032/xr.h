#ifndef FE_XR_H
#define FE_XR_H

#include <assert.h>
#include "iron/iron.h"

typedef union {
    usize imm;
    FeSymbol* sym;
} XrInstImmOrSym;

static inline bool xr_immsym_is_imm(XrInstImmOrSym* immsym) {
    return immsym->imm & 1;
}
static inline u32 xr_immsym_imm_val(XrInstImmOrSym* immsym) {
    return (u32)(immsym->imm >> 1);
}
static inline void xr_immsym_set_imm(XrInstImmOrSym* immsym, u32 imm) {
    immsym->imm = 1 | (imm << 1);
}

typedef struct {
    u16 imm;
    u8 small; // u5
    u8 xsh;   // u5
} XrInstImm;

typedef struct {
    FeBlock* if_true;
    FeBlock* if_false; // "fake" branch target
} XrInstBranch;

typedef enum : FeInstKind {
    XR__INVALID = FE__XR_INST_BEGIN,

    // Jumps
    XR_J,   // Jump
    XR_JAL, // Jump And Link
    
    // Branches
    XR_BEQ, // Branch Equal
    XR_BNE, // Branch Not Equal
    XR_BLT, // Branch Less Than
    XR_BLE, // Branch Less Than or Equal
    XR_BGT, // Branch Greater Than
    XR_BGE, // Branch Greater Than or Equal
    XR_BPE, // Branch Parity Even (lowest bit 0)
    XR_BPO, // Branch Parity Odd (lowest bit 1)

    // Immediate Operate
    XR_ADDI,    // Add Immediate
    XR_SUBI,    // Subtract Immediate
    XR_SLTI,    // Set Less Tahn Immediate
    XR_SLTI_S,  // Set Less Than Immediate, Signed
    XR_ANDI,    // And Immediate
    XR_ORI,     // Or Immediate
    XR_LUI,     // Load Upper Immediate

    XR_LOAD8_IO,      // Load Byte, Immediate Offset
    XR_LOAD16_IO,     // Load Int, Immediate Offset
    XR_LOAD32_IO,     // Load Word, Immediate Offset
    XR_STORE8_IO,     // Store Byte, Immediate Offset
    XR_STORE16_IO,    // Store Int, Immediate Offset
    XR_STORE32_IO,    // Store Word, Immediate Offset

    XR_LOAD8_RO,      // Load Byte, Register Offset
    XR_LOAD16_RO,     // Load Int, Register Offset
    XR_LOAD32_RO,     // Load Word, Register Offset
    XR_STORE8_RO,     // Store Byte, Register Offset
    XR_STORE16_RO,    // Store Int, Register Offset
    XR_STORE32_RO,    // Store Word, Register Offset

    XR_STORE8_SI,   // Store Byte, Small Immediate
    XR_STORE16_SI,  // Store Int, Small Immediate
    XR_STORE32_SI,  // Store Word, Small Immediate

    // Register Operate
    XR_LSH,     // Left Shift By Register Amount
    XR_RSH,     // Logical Right Shift By Register Amount
    XR_ASH,     // Arithmetic Right Shift By Register Amount
    XR_ADD,     // Add Register
    XR_SUB,     // Subtract Register
    XR_SLT,     // Set Less than Register
    XR_SLT_S,   // Set Less than Register, Signed
    XR_AND,     // And Register
    XR_XOR,     // Xor Register
    XR_OR,      // Or Register
    XR_NOR,     // Nor Register

    XR_MUL,     // Multiply
    XR_DIV,     // Divide
    XR_DIV_S,   // Divide Signed
    XR_MOD,     // Modulo

    XR_LL,      // Load Locked
    XR_SC,      // Store Conditional
    XR_MB,      // Memory Barrier
    XR_WMB,     // Write Memory Barrier
    XR_BRK,     // Breakpoint
    XR_SYS,     // System Service

    // Privileged Instructions
    XR_MFCR,    // Move From Control Register
    XR_MTCR,    // Move To Control Register
    XR_HLT,     // Halt Until Next Interrupt
    XR_RFE,     // Return From Exception
} XrInstKind;

typedef enum : FeRegClass {
    XR_REGCLASS_NONE = FE_REGCLASS_NONE,
    XR_REGCLASS_GPR,
} XrRegClasses;

typedef enum : u16 {
    XR_GPR_ZERO = 0,
    // temps (call-clobbered)
    XR_GPR_T0, XR_GPR_T1, XR_GPR_T2, XR_GPR_T3, XR_GPR_T4, XR_GPR_T5,
    // args (call-clobbered)
    XR_GPR_A0, XR_GPR_A1, XR_GPR_A2, XR_GPR_A3,
    // local vars (call-preserved)
    XR_GPR_S0, XR_GPR_S1, XR_GPR_S2, XR_GPR_S3, XR_GPR_S4,
    XR_GPR_S5, XR_GPR_S6, XR_GPR_S7, XR_GPR_S8, XR_GPR_S9,
    XR_GPR_S10, XR_GPR_S11, XR_GPR_S12, XR_GPR_S13, XR_GPR_S14,
    XR_GPR_S15, XR_GPR_S16, XR_GPR_S17,

    // thread pointer (dont touch!)
    XR_GPR_TP,
    // stack pointer (sometimes touch!)
    XR_GPR_SP,
    // link register (only touch in specific circumstances!)
    XR_GPR_LR,
} XrGpr;

FeInstChain fe_xr_isel(FeFunc* f, FeBlock* block, FeInst* inst);
FeRegClass fe_xr_choose_regclass(FeInstKind kind, FeTy ty);

#endif
