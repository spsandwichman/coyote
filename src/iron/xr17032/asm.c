#include "iron/iron.h"
#include "xr.h"

#define DONT_EMIT 1

// static u16 reg_num(FeFunc* f, FeInst* inst) {
//     FeVirtualReg* vr = fe_vreg(f->vregs, inst->vr_out);
//     return vr->real;
// }

static const char* reg(FeFunc* f, FeInst* inst) {
    if (inst->vr_out == FE_VREG_NONE) {
        return "FE_VREG_NONE";
    }
    FeVirtualReg* vr = fe_vreg(f->vregs, inst->vr_out);
    if (vr->real == FE_VREG_REAL_UNASSIGNED) {
        return "FE_VREG_REAL_UNASSIGNED";
    }
    return xr_reg_name(vr->class, vr->real);
}

static void emit_block_name(FeDataBuffer* db, FeBlock* b) {
    fe_db_writef(db, ".b%u", b->flags);
}

static void emit_inst_name(FeDataBuffer* db, u16 kind) {
    const char* name = xr_inst_name(kind, false);
    usize len = strlen(name);

    fe_db_writecstr(db, "    ");
    fe_db_writecstr(db, name);
    for_n(i, len, 4) {
        fe_db_write8(db, ' ');
    }
    fe_db_write8(db, ' ');
}

static void emit_reg_imm16(FeFunc* f, FeDataBuffer* db, FeInst* inst) {
    emit_inst_name(db, inst->kind);
    fe_db_writecstr(db, reg(f, inst));
    fe_db_writecstr(db, ", ");
    XrRegImm16* ri = fe_extra(inst);
    fe_db_writecstr(db, reg(f, ri->reg));
    fe_db_writef(db, ", %u", ri->imm16);
}

static void emit_reg_reg(FeFunc* f, FeDataBuffer* db, FeInst* inst) {
    emit_inst_name(db, inst->kind);
    fe_db_writecstr(db, reg(f, inst));
    fe_db_writecstr(db, ", ");

    XrRegReg* rr = fe_extra(inst);
    fe_db_writecstr(db, reg(f, rr->r1));
    fe_db_writecstr(db, ", ");
    fe_db_writecstr(db, reg(f, rr->r2));
}

static void emit_shift_group(FeDataBuffer* db, XrShiftKind shift_kind, u8 imm5) {
    if (imm5 != 0) switch (shift_kind) {
    case XR_SHIFT_LSH: fe_db_writef(db, " LSH %u", imm5); break;
    case XR_SHIFT_RSH: fe_db_writef(db, " RSH %u", imm5); break;
    case XR_SHIFT_ASH: fe_db_writef(db, " ASH %u", imm5); break;
    case XR_SHIFT_ROR: fe_db_writef(db, " ROR %u", imm5); break;
    }
}

static void emit_branch(FeFunc* f, FeBlock* b, FeDataBuffer* db, FeInst* inst) {
    // figure out if there's a block next.
    XrRegBranch* br = fe_extra(inst);

    emit_inst_name(db, inst->kind);
    fe_db_writecstr(db, reg(f, br->reg));
    fe_db_writecstr(db, ", ");
    emit_block_name(db, br->dest);
}

static char* mem_operand_size(FeInstKind kind) {
    auto xr_kind = (XrInstKind)kind;
    switch (xr_kind) {
    case XR_LOAD8_IMM:
    case XR_LOAD8_REG:
    case XR_STORE8_CONST:
    case XR_STORE8_IMM:
    case XR_STORE8_REG:
        return "byte";
    case XR_LOAD16_IMM:
    case XR_LOAD16_REG:
    case XR_STORE16_CONST:
    case XR_STORE16_IMM:
    case XR_STORE16_REG:
        return "int";
    case XR_LOAD32_IMM:
    case XR_LOAD32_REG:
    case XR_STORE32_CONST:
    case XR_STORE32_IMM:
    case XR_STORE32_REG:
        return "long";
    default: 
        return "";
    }
}

static void emit_mem_operand(FeFunc* f, FeDataBuffer* db, FeInst* inst) {
    FeInst* base;
    FeInst* shifted;
    XrShiftKind shift;
    u8 shamt;

    switch ((XrInstKind)inst->kind) {
    case XR_LOAD8_REG ... XR_LOAD32_REG:
        base = fe_extra_T(inst, XrRegReg)->r1;
        shifted = fe_extra_T(inst, XrRegReg)->r2;
        shift = fe_extra_T(inst, XrRegReg)->shift_kind;
        shamt = fe_extra_T(inst, XrRegReg)->imm5;
        break;
    case XR_STORE8_REG ... XR_STORE32_REG:
        base = fe_extra_T(inst, XrRegRegReg)->r1;
        shifted = fe_extra_T(inst, XrRegRegReg)->r2;
        shift = fe_extra_T(inst, XrRegRegReg)->shift_kind;
        shamt = fe_extra_T(inst, XrRegRegReg)->imm5;
        break;
    default:
        fe_runtime_crash("incorrect inst type for emit_mem_operand");
    }
    FeVirtualReg* base_vr = fe_vreg(f->vregs, base->vr_out);
    FeVirtualReg* shifted_vr = fe_vreg(f->vregs, shifted->vr_out);

    fe_db_writecstr(db, mem_operand_size(inst->kind));
    fe_db_writecstr(db, " [");
    if (base_vr->real != XR_REG_ZERO) {
        fe_db_writecstr(db, reg(f, base));
    }
    if (base_vr->real != XR_REG_ZERO && shifted_vr->real != XR_REG_ZERO) {
        fe_db_writecstr(db, " + ");
    }
    if (shifted_vr->real != XR_REG_ZERO) {
        fe_db_writecstr(db, reg(f, shifted));
        emit_shift_group(db, shift, shamt);
    }

    fe_db_writecstr(db, "]");
}

static void emit_inst(FeFunc* f, FeBlock* b, FeDataBuffer* db, FeInst* inst) {
    switch ((XrInstKind)inst->kind) {
    case XR_MOV: {
        emit_inst_name(db, inst->kind);
        fe_db_writecstr(db, reg(f, inst));
        fe_db_writecstr(db, ", ");
        XrRegImm16* ri = fe_extra(inst);
        fe_db_writecstr(db, reg(f, ri->reg));
        break;
    }
    case XR_ADDI ... XR_LUI:
        emit_reg_imm16(f, db, inst);
        break;
    case XR_ADD ... XR_MOD:
        emit_reg_reg(f, db, inst);
        emit_shift_group(db, fe_extra_T(inst, XrRegReg)->shift_kind, fe_extra_T(inst, XrRegReg)->imm5);
        break;
    case XR_STORE8_IMM ... XR_STORE32_IMM:
        emit_inst_name(db, inst->kind);
        fe_db_writef(db, "%s [%s",
            mem_operand_size(inst->kind),
            reg(f, fe_extra_T(inst, XrRegRegImm16)->r1)
        );
        if (fe_extra_T(inst, XrRegRegImm16)->imm16 != 0) {
            fe_db_writef(db, " + %u",
                fe_extra_T(inst, XrRegRegImm16)->imm16
            );
        }
        fe_db_writef(db, "], %s",
            reg(f, fe_extra_T(inst, XrRegRegImm16)->r2)
        );
        break;
    case XR_LOAD8_IMM ... XR_LOAD32_IMM:
        emit_inst_name(db, inst->kind);
        fe_db_writef(db, "%s, %s [%s",
            reg(f, inst),
            mem_operand_size(inst->kind),
            reg(f, fe_extra_T(inst, XrRegImm16)->reg)
        );
        if (fe_extra_T(inst, XrRegImm16)->imm16 != 0) {
            fe_db_writef(db, " + %u",
                fe_extra_T(inst, XrRegImm16)->imm16
            );
        }
        fe_db_writecstr(db, "]");
        break;
    case XR_BEQ ... XR_BGE:
        emit_branch(f, b, db, inst);
        break;
    case XR_J:
        emit_inst_name(db, inst->kind);
        // fe_db_writecstr(db, " ");
        emit_block_name(db, fe_extra_T(inst, XrJump)->dest);
        break;
    case XR_RET:
        emit_inst_name(db, inst->kind);
        break;
    default:
        fe_db_writef(db, "    ??? %d", inst->kind);
        break;
    }
    fe_db_writecstr(db, "\n");
}

void xr_emit_assembly(FeDataBuffer* db, FeFunc* f) {
    // number all instructions and blocks
    u32 block_counter = 0;
    for_blocks(block, f) {
        block->flags = block_counter++;
        for_inst(inst, block) {
            inst->flags = 0;
        }
    }

    // function label
    fe_db_write(db, f->sym->name, f->sym->name_len);
    fe_db_writecstr(db, ":\n");

    for_blocks(block, f) {
        if (block->flags != 0) {
            emit_block_name(db, block);
            fe_db_writecstr(db, "\n");
        }

        for_inst(inst, block) {
            if (inst->flags == DONT_EMIT) {
                continue;
            }
            emit_inst(f, block, db, inst);
        }
    }
}