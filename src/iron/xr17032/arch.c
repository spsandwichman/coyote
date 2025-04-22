#include "iron/iron.h"
#include "xr.h"

#include "../short_traits.h"

#define R(a, b) [a - _FE_XR_INST_BEGIN ... b - _FE_XR_INST_BEGIN]
#define I(a) [a - _FE_XR_INST_BEGIN]
    u8 xr_size_table[_FE_XR_INST_END - _FE_XR_INST_BEGIN] = {
        [0 ... 255] = 255,
        R(XR_ADDI, XR_LOAD32_IMM) = sizeof(XrRegImm16),
        R(XR_STORE8_IMM, XR_STORE32_IMM) = sizeof(XrRegRegImm16),
        R(XR_SHIFT, XR_LOAD32_REG) = sizeof(XrRegReg),
        R(XR_STORE8_REG, XR_STORE32_REG) = sizeof(XrRegRegReg),
        R(XR_BEQ, XR_BGE) = sizeof(XrRegBranch),
        I(XR_RET)   = 0,
    };
    FeTrait xr_trait_table[_FE_XR_INST_END - _FE_XR_INST_BEGIN] = {
        // TODO add more traits here.
        I(XR_ADDI) = INT_IN | SAME_IN_OUT,
        I(XR_SUBI) = INT_IN | SAME_IN_OUT,
        I(XR_ADD)  = INT_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
        I(XR_SUB)  = INT_IN | SAME_IN_OUT | SAME_INS,
        I(XR_MUL)  = INT_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,

        R(XR_STORE8_IMM, XR_STORE32_IMM) = VOL,
        R(XR_STORE8_REG, XR_STORE32_REG) = VOL,

        R(XR_BEQ, XR_BGE) = TERM,
        I(XR_RET)         = TERM,
    };
#undef I
#undef R

u16 xr_regclass_lens[] = {
    [0] = 0,
    [XR_REGCLASS_REG] = XR_REG__COUNT,
};

const char* xr_inst_name(FeInstKind kind, bool ir) {
    XrInstKind xrkind = (XrInstKind) kind;
    switch (xrkind) {
    case XR_ADDI: return ir ? "xr.addi": "addi";
    case XR_SUBI: return ir ? "xr.subi": "subi";
    case XR_SLTI: return ir ? "xr.slti": "slti";
    case XR_SLTI_SIGNED: return ir ? "xr.slti_signed": "slti signed";
    case XR_ANDI: return ir ? "xr.slti": "slti";
    case XR_XORI: return ir ? "xr.xori": "xori";
    case XR_ORI:  return ir ? "xr.ori" : "ori";
    case XR_LUI:  return ir ? "xr.lui" : "lui";
    case XR_MOV:  return ir ? "xr.mov" : "mov";
    case XR_LOAD8_IMM:  return ir ? "xr.load8_imm"  : "mov";
    case XR_LOAD16_IMM: return ir ? "xr.load16_imm" : "mov";
    case XR_LOAD32_IMM: return ir ? "xr.load32_imm" : "mov";

    case XR_STORE8_IMM:  return ir ? "xr.store8_imm"  : "mov";
    case XR_STORE16_IMM: return ir ? "xr.store16_imm" : "mov";
    case XR_STORE32_IMM: return ir ? "xr.store32_imm" : "mov";

    case XR_STORE8_CONST:  return ir ? "xr.store8_const"  : "mov";
    case XR_STORE16_CONST: return ir ? "xr.store16_const" : "mov";
    case XR_STORE32_CONST: return ir ? "xr.store32_const" : "mov";

    case XR_SHIFT: return ir ? "xr.shift" : ""; // gets special printing in assembly
    case XR_ADD: return ir ? "xr.add" : "add";
    case XR_SUB: return ir ? "xr.sub" : "sub";
    case XR_SLT: return ir ? "xr.slt" : "slt";
    case XR_SLT_SIGNED: return ir ? "xr.slt_signed" : "slt signed";
    case XR_AND: return ir ? "xr.and" : "and";
    case XR_XOR: return ir ? "xr.xor" : "xor";
    case XR_OR:  return ir ? "xr.or"  : "or";
    case XR_NOR: return ir ? "xr.nor" : "nor";
    case XR_LOAD8_REG:  return ir ? "xr.load8_reg"  : "mov";
    case XR_LOAD16_REG: return ir ? "xr.load16_reg" : "mov";
    case XR_LOAD32_REG: return ir ? "xr.load32_reg" : "mov";
    case XR_MUL: return ir ? "xr.mul" : "mul";
    case XR_DIV: return ir ? "xr.div" : "div";
    case XR_DIV_SIGNED: return ir ? "xr.div_signed" : "div signed";
    case XR_MOD: return ir ? "xr.mod" : "mod";

    case XR_STORE8_REG:  return ir ? "xr.store8_reg"  : "mov";
    case XR_STORE16_REG: return ir ? "xr.store16_reg" : "mov";
    case XR_STORE32_REG: return ir ? "xr.store32_reg" : "mov";

    case XR_BEQ: return ir ? "xr.beq" : "beq";
    case XR_BNE: return ir ? "xr.bne" : "bne";
    case XR_BLT: return ir ? "xr.blt" : "blt";
    case XR_BGT: return ir ? "xr.bgt" : "bgt";
    case XR_BLE: return ir ? "xr.ble" : "ble";
    case XR_BGE: return ir ? "xr.bge" : "bge";

    case XR_RET: return ir ? "xr.ret" : "ret";
    default:
        return "xr.???";
    }
}

const char* xr_reg_name(u8 regclass, u16 real) {
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
    default:
        return "???";
    }
    
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
    switch ((XrInstKind)inst->kind) {  
    case XR_ADDI ... XR_LOAD32_IMM:
        fe__print_ref(f, db, fe_extra_T(inst, XrRegImm16)->reg);
        if (inst->kind != XR_MOV) 
            fe_db_writef(db, ", 0x%x", (u16)fe_extra_T(inst, XrRegImm16)->imm16);
        break;    
    case XR_SHIFT ... XR_LOAD32_REG:
        fe__print_ref(f, db, fe_extra_T(inst, XrRegReg)->r1);
        fe_db_writecstr(db, ", ");
        fe__print_ref(f, db, fe_extra_T(inst, XrRegReg)->r2);
        if (fe_extra_T(inst, XrRegReg)->imm5 != 0) {
            switch (fe_extra_T(inst, XrRegReg)->shift_kind) {
            case XR_SHIFT_LSH: fe_db_writef(db, " lsh %u", fe_extra_T(inst, XrRegReg)->imm5); break;
            case XR_SHIFT_RSH: fe_db_writef(db, " rsh %u", fe_extra_T(inst, XrRegReg)->imm5); break;
            case XR_SHIFT_ASH: fe_db_writef(db, " ash %u", fe_extra_T(inst, XrRegReg)->imm5); break;
            case XR_SHIFT_ROR: fe_db_writef(db, " ror %u", fe_extra_T(inst, XrRegReg)->imm5); break;
            }
        }
        break;
    case XR_BEQ ... XR_BGE:
        fe__print_ref(f, db, fe_extra_T(inst, XrRegBranch)->reg);
        fe_db_writecstr(db, ", ");
        fe__print_block(f, db, fe_extra_T(inst, XrRegBranch)->dest);
        fe_db_writecstr(db, " (else ");
        fe__print_block(f, db, fe_extra_T(inst, XrRegBranch)->_else);
        fe_db_writecstr(db, ")");
        break;
    case XR_STORE8_IMM ... XR_STORE32_IMM:
        fe__print_ref(f, db, fe_extra_T(inst, XrRegRegImm16)->r1);
        fe_db_writecstr(db, ", ");
        fe__print_ref(f, db, fe_extra_T(inst, XrRegRegImm16)->r2);
        fe_db_writef(db, ", 0x%x", (u16)fe_extra_T(inst, XrRegRegImm16)->imm16);
        break;
    case XR_STORE8_REG ... XR_STORE32_REG:
        fe__print_ref(f, db, fe_extra_T(inst, XrRegRegReg)->r1);
        fe_db_writecstr(db, ", ");
        fe__print_ref(f, db, fe_extra_T(inst, XrRegRegReg)->r2);
        if (fe_extra_T(inst, XrRegRegReg)->imm5 != 0) {
            switch (fe_extra_T(inst, XrRegRegReg)->shift_kind) {
            case XR_SHIFT_LSH: fe_db_writef(db, " lsh %u", fe_extra_T(inst, XrRegRegReg)->imm5); break;
            case XR_SHIFT_RSH: fe_db_writef(db, " rsh %u", fe_extra_T(inst, XrRegRegReg)->imm5); break;
            case XR_SHIFT_ASH: fe_db_writef(db, " ash %u", fe_extra_T(inst, XrRegRegReg)->imm5); break;
            case XR_SHIFT_ROR: fe_db_writef(db, " ror %u", fe_extra_T(inst, XrRegRegReg)->imm5); break;
            }
        }
        fe_db_writecstr(db, ", ");
        fe__print_ref(f, db, fe_extra_T(inst, XrRegRegReg)->r3);
        break;
    case XR_RET:
        break;
    default:
        fe_db_writecstr(db, "???");
    }
}

FeInst** xr_list_inputs(FeInst* inst, usize* len_out) {
    switch ((XrInstKind)inst->kind) {
    case XR_ADDI ... XR_LOAD32_IMM:
        *len_out = 1;
        return &fe_extra_T(inst, XrRegImm16)->reg;
    case XR_SHIFT ... XR_LOAD32_REG:
        *len_out = 2;
        return &fe_extra_T(inst, XrRegReg)->r1;
    case XR_STORE8_REG ... XR_STORE32_REG:
        *len_out = 3;
        return &fe_extra_T(inst, XrRegRegReg)->r1;
    case XR_BEQ ... XR_BGE:
        *len_out = 1;
        return &fe_extra_T(inst, XrRegBranch)->reg;
    case XR_RET:
        *len_out = 0;
        return nullptr;
    default:
        fe_runtime_crash("xr_list_inputs: unknown kind %d", inst->kind);
        break;
    }
}

FeBlock** xr_term_list_targets(FeInst* term, usize* len_out) {
    switch (term->kind) {
    case XR_BEQ:
    case XR_BNE:
    case XR_BLT:
    case XR_BGT:
    case XR_BLE:
    case XR_BGE:
        *len_out = 2;
        return &fe_extra_T(term, XrRegBranch)->dest;
    case XR_RET:
        *len_out = 0;
        return nullptr;
    default:
        fe_runtime_crash("xr_list_targets: unknown kind %d", term->kind);
        break;
    }
}

FeRegclass xr_choose_regclass(FeInstKind kind, FeTy ty) {
    return XR_REGCLASS_REG; // BAD
}