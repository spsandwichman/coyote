#include "iron/iron.h"

usize fe_xr_extra_size_unsafe(FeInstKind kind) {
    switch (kind) {
    case FE_XR_ADDI:
    case FE_XR_LUI:
    case FE_XR_SUBI:
        return sizeof(FeXrRegImm16);
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        return sizeof(FeXrRegReg);
    case FE_XR_RET:
        return 0;
    default:
        return 255;
    }
}

char* fe_xr_inst_name(FeInstKind kind) {
    switch (kind) {
    case FE_XR_ADDI: return "xr.addi";
    case FE_XR_LUI:  return "xr.lui";
    case FE_XR_SUBI: return "xr.subi";

    case FE_XR_ADD: return "xr.add";
    case FE_XR_SUB: return "xr.sub";
    case FE_XR_MUL: return "xr.mul";

    case FE_XR_BEQ: return "xr.beq";
    case FE_XR_BNE: return "xr.bne";
    case FE_XR_BLT: return "xr.blt";
    case FE_XR_BGT: return "xr.bgt";
    case FE_XR_BLE: return "xr.ble";
    case FE_XR_BGE: return "xr.bge";

    case FE_XR_RET: return "xr.ret";
    }
    return "xr.???";
}

char* fe_xr_reg_name(u16 real) {
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
    case XR_REG_A4: return "a4";

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

void fe_xr_print_args(FeDataBuffer* db, FeInst* inst) {
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_LUI:
    case FE_XR_SUBI:
        fe__print_ref(db, fe_extra_T(inst, FeXrRegImm16)->reg);
        fe_db_writef(db, ", 0x%x", (u16)fe_extra_T(inst, FeXrRegImm16)->num);
        break;    
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        fe__print_ref(db, fe_extra_T(inst, FeXrRegReg)->lhs);
        fe_db_writecstr(db, ", ");
        fe__print_ref(db, fe_extra_T(inst, FeXrRegReg)->rhs);
        break;    
    case FE_XR_BEQ:
    case FE_XR_BNE:
    case FE_XR_BLT:
    case FE_XR_BGT:
    case FE_XR_BLE:
    case FE_XR_BGE:
        fe__print_ref(db, fe_extra_T(inst, FeXrRegBranch)->reg);
        fe_db_writecstr(db, ", ");
        fe__print_block(db, fe_extra_T(inst, FeXrRegBranch)->dest);
        fe_db_writecstr(db, " (else ");
        fe__print_block(db, fe_extra_T(inst, FeXrRegBranch)->_else);
        fe_db_writecstr(db, ")");
        break;
    case FE_XR_RET:
        break;
    default:
        fe_db_writecstr(db, "???");
    }
}

FeInst** fe_xr_list_inputs(FeInst* inst, usize* len_out) {
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_LUI:
    case FE_XR_SUBI:
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

FeBlock** fe_xr_term_list_targets(FeInst* term, usize* len_out) {
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