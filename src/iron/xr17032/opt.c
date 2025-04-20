#include "iron/iron.h"
#include "xr.h"

static FeInst* peephole(FeInst* inst) {
    FeXrRegImm16* reg_imm16 = fe_extra(inst);
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_SUBI:
    case FE_XR_LUI:
        if (reg_imm16->num == 0) {
            return reg_imm16->reg;
        }
        break;
    }
    return inst;
}

void xr_opt(FeFunction* f) {
    FeTarget* t = f->mod->target;

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