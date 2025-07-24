#include "iron/iron.h"

// TDCE - trivial dead code elimination

// rewrite this lmao, i dont know how this became this bad

#define TDCE_DEAD_LMAO 0xDEADDEAD

void fe_opt_tdce(FeFunc* f) {
    const FeTarget* t = f->mod->target;

    // TODO fuck all these worklists into OUTERED SPACE
    FeWorklist wl;

    for_blocks(block, f) {
        for_inst(inst, block) {
            fe_wl_push(&wl, inst);
        }
    }

    while (wl.len != 0) {
        FeInst* inst = fe_wl_pop(&wl);
        // if (inst->kind == FE_PARAM) {
            // continue;
        // }

        if (inst->use_len == 0 && !fe_inst_has_trait(inst->kind, FE_TRAIT_VOLATILE)) {
            fe_inst_destroy(f, inst);
        }
    }

    fe_wl_destroy(&wl);
}
