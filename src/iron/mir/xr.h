#ifndef FE_MIR_XR_H
#define FE_MIR_XR_H

#include "iron/iron.h"
#include "mir.h"

typedef enum : u8 {
    MIR_XR_GPR_ZERO = 0,

    // temps (call-clobbered)
    MIR_XR_GPR_T0, MIR_XR_GPR_T1, MIR_XR_GPR_T2, MIR_XR_GPR_T3, MIR_XR_GPR_T4, MIR_XR_GPR_T5,
    
    // args (call-clobbered)
    MIR_XR_GPR_A0, MIR_XR_GPR_A1, MIR_XR_GPR_A2, MIR_XR_GPR_A3,
    
    // local vars (call-preserved)
    MIR_XR_GPR_S0, MIR_XR_GPR_S1, MIR_XR_GPR_S2, MIR_XR_GPR_S3, MIR_XR_GPR_S4,
    MIR_XR_GPR_S5, MIR_XR_GPR_S6, MIR_XR_GPR_S7, MIR_XR_GPR_S8, MIR_XR_GPR_S9,
    MIR_XR_GPR_S10, MIR_XR_GPR_S11, MIR_XR_GPR_S12, MIR_XR_GPR_S13, MIR_XR_GPR_S14,
    MIR_XR_GPR_S15, MIR_XR_GPR_S16, MIR_XR_GPR_S17,

    // thread pointer (dont touch!)
    MIR_XR_GPR_TP,
    
    // stack pointer (sometimes touch!)
    MIR_XR_GPR_SP,
    
    // link register (only touch in specific circumstances!)
    MIR_XR_GPR_LR,
} MirXrGpr;

typedef enum : u8 {
    MIR_XR_J,   // Jump
    MIR_XR_JAL, // Jump And Link
    
    // Branches
    MIR_XR_BEQ,     // Branch Equal
    MIR_XR_BNE,     // Branch Not Equal
    MIR_XR_BLT,     // Branch Less Than
    MIR_XR_BLE,     // Branch Less Than or Equal
    MIR_XR_BGT,     // Branch Greater Than
    MIR_XR_BGE,     // Branch Greater Than or Equal
    MIR_XR_BPE,     // Branch Parity Even (lowest bit 0)
    MIR_XR_BPO,     // Branch Parity Odd (lowest bit 1)

    // Immediate Operate
    MIR_XR_ADDI,    // Add Immediate
    MIR_XR_SUBI,    // Subtract Immediate
    MIR_XR_SLTI,    // Set Less Than Immediate
    MIR_XR_SLTI_S,  // Set Less Than Immediate, Signed
    MIR_XR_ANDI,    // And Immediate
    MIR_XR_ORI,     // Or Immediate
    MIR_XR_LUI,     // Load Upper Immediate
    MIR_XR_JALR,    // Jump And Link, Register

    MIR_XR_LOAD8_IO,      // Load Byte, Immediate Offset
    MIR_XR_LOAD16_IO,     // Load Int, Immediate Offset
    MIR_XR_LOAD32_IO,     // Load Long, Immediate Offset
    MIR_XR_STORE8_IO,     // Store Byte, Immediate Offset
    MIR_XR_STORE16_IO,    // Store Int, Immediate Offset
    MIR_XR_STORE32_IO,    // Store Long, Immediate Offset

    MIR_XR_LOAD8_RO,      // Load Byte, Register Offset
    MIR_XR_LOAD16_RO,     // Load Int, Register Offset
    MIR_XR_LOAD32_RO,     // Load Long, Register Offset
    MIR_XR_STORE8_RO,     // Store Byte, Register Offset
    MIR_XR_STORE16_RO,    // Store Int, Register Offset
    MIR_XR_STORE32_RO,    // Store Long, Register Offset

    MIR_XR_STORE8_SI,   // Store Byte, Small Immediate
    MIR_XR_STORE16_SI,  // Store Int, Small Immediate
    MIR_XR_STORE32_SI,  // Store Long, Small Immediate

    // Register Operate
    MIR_XR_SHIFT,   // Various Shift By Register Amount
    MIR_XR_ADD,     // Add Register
    MIR_XR_SUB,     // Subtract Register
    MIR_XR_SLT,     // Set Less Than Register
    MIR_XR_SLT_S,   // Set Less Than Register, Signed
    MIR_XR_AND,     // And Register
    MIR_XR_XOR,     // Xor Register
    MIR_XR_OR,      // Or Register
    MIR_XR_NOR,     // Nor Register

    MIR_XR_MUL,     // Multiply
    MIR_XR_DIV,     // Divide
    MIR_XR_DIV_S,   // Divide Signed
    MIR_XR_MOD,     // Modulo

    MIR_XR_LL,      // Load Locked
    MIR_XR_SC,      // Store Conditional

    // void
    MIR_XR_MB,      // Memory Barrier
    MIR_XR_WMB,     // Write Memory Barrier
    MIR_XR_BRK,     // Breakpoint
    MIR_XR_SYS,     // System Service

    // Privileged Instructions
    MIR_XR_MFCR,    // Move From Control Register
    MIR_XR_MTCR,    // Move To Control Register
    MIR_XR_HLT,     // Halt Until Next Interrupt
    MIR_XR_RFE,     // Return From Exception
} MirXrInstKind;

typedef enum : u8 {
    MIR_XR_RELOC_PTR,       // pointer
    MIR_XR_RELOC_ABSJ,      // absolute jump
    MIR_XR_RELOC_LA,        // LA pseudo-inst
    MIR_XR_RELOC_FAR_INT,   // far-int access psuedo-inst
    MIR_XR_RELOC_FAR_LONG,  // far-long access psuedo-inst
} MirXrRelocKind;

typedef struct MirXrInst {
    MirXrInstKind kind;
    MirXrGpr ra;
    MirXrGpr rb;
    MirXrGpr rc;
    union {
        struct {
            u8 xsh;
            u8 shamt;
        };    
        u32 imm;
    };
} MirXrInst;
int x = sizeof(MirXrInst);

typedef struct MirXrRelocation {
    MirXrRelocKind kind;
    FeSymbol* symbol_ref;
} MirXrRelocation;

#endif // FE_MIR_XR_H