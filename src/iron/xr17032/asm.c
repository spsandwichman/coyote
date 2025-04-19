#include "iron/iron.h"

#define DONT_EMIT 1

static u16 reg_num(FeFunction* f, FeInst* inst) {
    FeVirtualReg* vr = fe_vreg(f->vregs, inst->vr_out);
    return vr->real;
}

static char* reg(FeFunction* f, FeInst* inst) {
    if (inst->vr_out == FE_VREG_NONE) {
        return "FE_VREG_NONE";
    }
    FeVirtualReg* vr = fe_vreg(f->vregs, inst->vr_out);
    if (vr->real == FE_VREG_REAL_UNASSIGNED) {
        return "FE_VREG_REAL_UNASSIGNED";
    }
    return fe_xr_reg_name(vr->real);
}

static void emit_block_name(FeDataBuffer* db, FeBlock* b) {
    fe_db_writef(db, ".b%u", b->flags);
}

static void emit_inst_name(FeDataBuffer* db, u16 kind) {
    char* name = fe_xr_inst_name(kind);
    usize len = strlen(name);

    fe_db_writecstr(db, "   ");
    fe_db_writecstr(db, name);
    for_n(i, len, 5) {
        fe_db_write8(db, ' ');
    }
    fe_db_write8(db, ' ');
}

static void emit_reg_imm16(FeFunction* f, FeDataBuffer* db, FeInst* inst) {
    emit_inst_name(db, inst->kind);
    fe_db_writecstr(db, reg(f, inst));
    fe_db_writecstr(db, ", ");
    FeXrRegImm16* ri = fe_extra(inst);
    fe_db_writecstr(db, reg(f, ri->reg));
    fe_db_writef(db, ", %u", ri->num);
}

static void emit_reg_reg(FeFunction* f, FeDataBuffer* db, FeInst* inst) {
    emit_inst_name(db, inst->kind);
    fe_db_writecstr(db, reg(f, inst));
    fe_db_writecstr(db, ", ");

    FeXrRegReg* rr = fe_extra(inst);
    fe_db_writecstr(db, reg(f, rr->lhs));
    fe_db_writecstr(db, ", ");
    fe_db_writecstr(db, reg(f, rr->rhs));
}

static void emit_branch(FeFunction* f, FeBlock* b, FeDataBuffer* db, FeInst* inst) {
    // figure out if there's a block next.
    FeXrRegBranch* br = fe_extra(inst);

    // if the _else block is right after the jump,
    // elide the uncondiitonal jump.
    if (br->_else->flags == b->flags + 1) {
        emit_inst_name(db, inst->kind);
        fe_db_writecstr(db, reg(f, br->reg));
        fe_db_writecstr(db, ", ");
        emit_block_name(db, br->dest);
    } else if (br->dest->flags == b->flags + 1) {
        // if the dest block is right after the jump,
        // flip the condition of the branch.
        emit_inst_name(db, inst->kind + 1); // lmao hacky
        fe_db_writecstr(db, reg(f, br->reg));
        fe_db_writecstr(db, ", ");
        emit_block_name(db, br->_else);
    } else {
        // emit both.
        emit_inst_name(db, inst->kind + 1);
        fe_db_writecstr(db, reg(f, br->reg));
        fe_db_writecstr(db, ", ");
        emit_block_name(db, br->dest);
        fe_db_writecstr(db, "\n");

        fe_db_writecstr(db, "j ");
        emit_block_name(db, br->_else);
    }
}

static void emit_inst(FeFunction* f, FeBlock* b, FeDataBuffer* db, FeInst* inst) {
    switch (inst->kind) {
    case FE_MOV_VOLATILE: {
        FeInst* input = fe_extra_T(inst, FeInstUnop)->un;
        fe_db_writecstr(db, "mov ");
        fe_db_writecstr(db, reg(f, inst));
        fe_db_writecstr(db, ", ");
        fe_db_writecstr(db, reg(f, input));
        break;
    }
    case FE_XR_ADDI:
    case FE_XR_LUI:
    case FE_XR_SUBI:
        emit_reg_imm16(f, db, inst);
        break;
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        emit_reg_reg(f, db, inst);
        break;
    case FE_XR_BEQ:
    case FE_XR_BNE:
    case FE_XR_BLT:
    case FE_XR_BGT:
    case FE_XR_BLE:
    case FE_XR_BGE:
        emit_branch(f, b, db, inst);
        break;
    case FE_XR_RET:
        emit_inst_name(db, inst->kind);
        break;
    default:
        fe_db_writecstr(db, "???");
        break;
    }
    fe_db_writecstr(db, "\n");
}

void fe_xr_emit_assembly(FeFunction* f, FeDataBuffer* db) {
    // number all instructions and blocks
    u32 block_counter = 0;
    for_blocks(block, f) {
        block->flags = block_counter++;
        for_inst(inst, block) {
            inst->flags = 0;
            if (inst->kind == FE_MACH_REG) {
                inst->flags = DONT_EMIT;
            }
            if (fe_inst_has_trait(inst->kind, FE_TRAIT_REG_MOV_HINT)) {
                FeInst* input = fe_extra_T(inst, FeInstUnop)->un;
                if (reg_num(f, inst) == reg_num(f, input)) {
                    inst->flags = DONT_EMIT;
                }
            }
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