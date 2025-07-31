#ifndef FE_XR_H
#define FE_XR_H

#include "iron/iron.h"

typedef struct {
    bool is_sym;
    union {
        u32 imm;
        FeInst* sym;
    };
} XrInstImmOrSym;

typedef struct {
    u32 imm;
} XrInstSmallImm;

typedef struct {
    FeBlock* if_true;
    FeBlock* if_false; // "fake" branch target
} XrInstBranch;

typedef enum : FeInstKind {
    XR_ADDI,
    XR_SUBI,
    XR_SLTI,
    XR_SLTI_SIGNED,
    XR_ANDI,
    XR_ORI,
    XR_LUI,

    XR_LOAD8I,
    XR_LOAD16I,
    XR_LOAD32I,
    XR_STORE8I,
    XR_STORE16I,
    XR_STORE32I,

    XR_STORE8I_IMM,
    XR_STORE16I_IMM,
    XR_STORE32I_IMM,
} XrInstKind;

#endif
