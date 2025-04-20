#include "iron/iron.h"
#include "xr.h"

#include "../short_traits.h"

#define R(a, b) [a - _FE_XR_INST_BEGIN ... b - _FE_XR_INST_BEGIN]
#define I(a) [a - _FE_XR_INST_BEGIN]
    u8 xr_size_table[_FE_XR_INST_END - _FE_XR_INST_BEGIN] = {
        [0 ... 255] = 255,
        R(FE_XR_ADDI, FE_XR_SLTI) = sizeof(FeXrRegImm16),
        R(FE_XR_ADD, FE_XR_MUL)   = sizeof(FeXrRegReg),
        R(FE_XR_BEQ, FE_XR_BGE)   = sizeof(FeXrRegBranch),

        I(FE_XR_RET)   = 0,
    };
    FeTrait xr_trait_table[_FE_XR_INST_END - _FE_XR_INST_BEGIN] = {
        I(FE_XR_ADDI) = INT_IN | SAME_IN_OUT,
        I(FE_XR_SUBI) = INT_IN | SAME_IN_OUT,
        I(FE_XR_ADD)  = INT_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
        I(FE_XR_SUB)  = INT_IN | SAME_IN_OUT | SAME_INS,
        I(FE_XR_MUL)  = INT_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
        R(FE_XR_BEQ, FE_XR_BGE) = VOL | TERM,
        I(FE_XR_RET)  = TERM,
    };
#undef I
#undef R

char* xr_inst_name(FeInstKind kind, bool ir) {
    switch (kind) {
    case FE_XR_ADDI: return ir ? "xr.addi": "addi";
    case FE_XR_LUI:  return ir ? "xr.lui" : "lui";
    case FE_XR_SUBI: return ir ? "xr.subi": "subi";
    case FE_XR_SLTI: return ir ? "xr.slti": "slti";

    case FE_XR_ADD: return ir ? "xr.add" : "add";
    case FE_XR_SUB: return ir ? "xr.sub" : "sub";
    case FE_XR_MUL: return ir ? "xr.mul" : "mul";

    case FE_XR_BEQ: return ir ? "xr.beq" : "beq";
    case FE_XR_BNE: return ir ? "xr.bne" : "bne";
    case FE_XR_BLT: return ir ? "xr.blt" : "blt";
    case FE_XR_BGT: return ir ? "xr.bgt" : "bgt";
    case FE_XR_BLE: return ir ? "xr.ble" : "ble";
    case FE_XR_BGE: return ir ? "xr.bge" : "bge";

    case FE_XR_RET: return ir ? "xr.ret" : "ret";
    }
    return "xr.???";
}

char* xr_reg_name(u8 regclass, u16 real) {
    if (regclass != XR_REGCLASS_REG || real >= XR_REG__COUNT) {
        return "???";
    }
    switch (real) {
    case XR_REG_ZERO: return "zero";

    case XR_REG_T0: return "t0";
    case XR_REG_T1: return "t1";
    case XR_REG_T2: return "t2";
    case XR_REG_T3: return "t3";
    case XR_REG_T4: return "t4";
    case XR_REG_T5: return "t5";

    case XR_REG_A0: return "a0";
    case XR_REG_A1: return "a1";
    case XR_REG_A2: return "a2";
    case XR_REG_A3: return "a3";

    case XR_REG_S0: return "s0";
    case XR_REG_S1: return "s1";
    case XR_REG_S2: return "s2";
    case XR_REG_S3: return "s3";
    case XR_REG_S4: return "s4";
    case XR_REG_S5: return "s5";
    case XR_REG_S6: return "s6";
    case XR_REG_S7: return "s7";
    case XR_REG_S8: return "s8";
    case XR_REG_S9: return "s9";
    case XR_REG_S10: return "s10";
    case XR_REG_S11: return "s11";
    case XR_REG_S12: return "s12";
    case XR_REG_S13: return "s13";
    case XR_REG_S14: return "s14";
    case XR_REG_S15: return "s15";
    case XR_REG_S16: return "s16";
    case XR_REG_S17: return "s17";

    case XR_REG_TP: return "tp";
    case XR_REG_SP: return "sp";
    case XR_REG_LR: return "lr";
    }
    return "???";
}

FeRegStatus xr_reg_status(u8 cconv, u8 regclass, u16 real) {
    (void)cconv;
    if (regclass != XR_REGCLASS_REG) {
        return FE_REG_UNUSABLE;
    }

    switch (real) {
    case XR_REG_ZERO:
        return FE_REG_UNUSABLE;
    case XR_REG_T0:
    case XR_REG_T1:
    case XR_REG_T2:
    case XR_REG_T3:
    case XR_REG_T4:
    case XR_REG_T5:
        return FE_REG_CALL_CLOBBERED;
    case XR_REG_A0:
    case XR_REG_A1:
    case XR_REG_A2:
    case XR_REG_A3:
        return FE_REG_CALL_CLOBBERED;
    case XR_REG_S0:
    case XR_REG_S1:
    case XR_REG_S2:
    case XR_REG_S3:
    case XR_REG_S4:
    case XR_REG_S5:
    case XR_REG_S6:
    case XR_REG_S7:
    case XR_REG_S8:
    case XR_REG_S9:
    case XR_REG_S10:
    case XR_REG_S11:
    case XR_REG_S12:
    case XR_REG_S13:
    case XR_REG_S14:
    case XR_REG_S15:
    case XR_REG_S16:
    case XR_REG_S17:
        return FE_REG_CALL_PRESERVED;
    case XR_REG_TP:
    case XR_REG_SP:
    case XR_REG_LR:
    default: 
        return FE_REG_UNUSABLE;
    }
}

void xr_print_args(FeFunction* f, FeDataBuffer* db, FeInst* inst) {
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_LUI:
    case FE_XR_SUBI:
    case FE_XR_SLTI:
        fe__print_ref(f, db, fe_extra_T(inst, FeXrRegImm16)->reg);
        fe_db_writef(db, ", 0x%x", (u16)fe_extra_T(inst, FeXrRegImm16)->num);
        break;    
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        fe__print_ref(f, db, fe_extra_T(inst, FeXrRegReg)->lhs);
        fe_db_writecstr(db, ", ");
        fe__print_ref(f, db, fe_extra_T(inst, FeXrRegReg)->rhs);
        break;    
    case FE_XR_BEQ:
    case FE_XR_BNE:
    case FE_XR_BLT:
    case FE_XR_BGT:
    case FE_XR_BLE:
    case FE_XR_BGE:
        fe__print_ref(f, db, fe_extra_T(inst, FeXrRegBranch)->reg);
        fe_db_writecstr(db, ", ");
        fe__print_block(f, db, fe_extra_T(inst, FeXrRegBranch)->dest);
        fe_db_writecstr(db, " (else ");
        fe__print_block(f, db, fe_extra_T(inst, FeXrRegBranch)->_else);
        fe_db_writecstr(db, ")");
        break;
    case FE_XR_RET:
        break;
    default:
        fe_db_writecstr(db, "???");
    }
}

FeInst** xr_list_inputs(FeInst* inst, usize* len_out) {
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_LUI:
    case FE_XR_SUBI:
    case FE_XR_SLTI:
        *len_out = 1;
        return &fe_extra_T(inst, FeXrRegImm16)->reg;
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        *len_out = 2;
        return &fe_extra_T(inst, FeXrRegReg)->lhs;   
    case FE_XR_BEQ:
    case FE_XR_BNE:
    case FE_XR_BLT:
    case FE_XR_BGT:
    case FE_XR_BLE:
    case FE_XR_BGE:
        *len_out = 1;
        return &fe_extra_T(inst, FeXrRegBranch)->reg;
    case FE_XR_RET:
        *len_out = 0;
        return NULL;
    default:
        fe_runtime_crash("xr_list_inputs: unknown kind %d", inst->kind);
        break;
    }
}

FeBlock** xr_term_list_targets(FeInst* term, usize* len_out) {
    switch (term->kind) {
    case FE_XR_BEQ:
    case FE_XR_BNE:
    case FE_XR_BLT:
    case FE_XR_BGT:
    case FE_XR_BLE:
    case FE_XR_BGE:
        *len_out = 2;
        return &fe_extra_T(term, FeXrRegBranch)->dest;
    case FE_XR_RET:
        *len_out = 0;
        return NULL;
    default:
        fe_runtime_crash("xr_list_targets: unknown kind %d", term->kind);
        break;
    }
}

FeRegclass xr_choose_regclass(FeInstKind kind, FeTy ty) {
    return XR_REGCLASS_REG; // BAD
}