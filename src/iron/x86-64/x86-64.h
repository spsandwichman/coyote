#ifndef FE_X86_64_H
#define FE_X86_64_H

#include "iron/iron.h"

enum : u8 {
    X64_GPR_RAX = 0b0000,
    X64_GPR_RBX = 0b0001,
    X64_GPR_RCX = 0b0010,
    X64_GPR_RDX = 0b0011,
    X64_GPR_RSP = 0b0100,
    X64_GPR_RBP = 0b0101,
    X64_GPR_RSI = 0b0110,
    X64_GPR_RDI = 0b0111,

    X64_GPR_R8  = 0b1000,
    X64_GPR_R9  = 0b1001,
    X64_GPR_R10 = 0b1010,
    X64_GPR_R11 = 0b1011,
    X64_GPR_R12 = 0b1100,
    X64_GPR_R13 = 0b1101,
    X64_GPR_R14 = 0b1110,
    X64_GPR_R15 = 0b1111,
};

enum {
    /*
        gpr
        [gpr]
        [gpr + i8]
        [gpr + i32]
        [gpr * scale]
        [gpr * scale + i8]
        [gpr * scale + i32]
        [gpr + gpr * scale]
        [gpr + gpr * scale + i8]
        [gpr + gpr * scale + i32]
    */

    X64_OP_REG,
    X64_OP_MEM,
} X64OperandKind;

// associated with a specific vreg
typedef struct {
    u8 mod;
    u8 scale;

    u32 disp;
} X64Operand;

#endif
