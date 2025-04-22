#include "iron/iron.h"
#include "xr.h"

static FeInst* peephole(FeInst* inst) {
    XrRegImm16* reg_imm16 = fe_extra(inst);
    switch (inst->kind) {
    case XR_ADDI:
    case XR_SUBI:
    case XR_LUI:
        if (reg_imm16->imm16 == 0) {
            return reg_imm16->reg;
        }
        break;
    }
    return inst;
}

void xr_pre_regalloc_opt(FeFunction* f) {
    const FeTarget* t = f->mod->target;

    for_blocks(b, f) {
        for_inst(inst, b) {
            usize len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &len);
            for_n (i, 0, len) {
                inputs[i] = peephole(inputs[i]);
            }
        }
    }
}