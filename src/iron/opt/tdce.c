#include "iron/iron.h"

// TDCE - trivial dead code elimination

// rewrite this lmao, i dont know how this became this bad

#define TDCE_DEAD_LMAO 0xDEADDEAD

void fe_opt_tdce(FeFunc* f) {
    const FeTarget* t = f->mod->target;

    // TODO fuck all these worklists into OUTERED SPACE
    FeWorklist wl;
    FeWorklist dead;
    FeWorklist to_free;
    fe_wl_init(&wl);
    fe_wl_init(&dead);
    fe_wl_init(&to_free);

    // update every instruction's use count
    fe_inst_calculate_uses(f);

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
            // get rid of it!!!!!!!!!!!!
            fe_wl_push(&dead, inst);
            inst->flags = TDCE_DEAD_LMAO;

            // decrement all input use counts
            usize len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &len);
            for_n (i, 0, len) {
                if (inputs[i]->flags == TDCE_DEAD_LMAO) {
                    continue;
                }
                fe_inst_unordered_remove_use(inputs[i], inst);
                if (inputs[i]->use_len == 0) {
                    fe_wl_push(&wl, inputs[i]);
                }
            }
        }
    }

    // get rid of dem
    for_n(i, 0, dead.len) {
        if (dead.at[i]->flags == 0) {
            continue;
        }
        fe_inst_remove_pos(dead.at[i]); // remove from block
        fe_wl_push(&to_free, dead.at[i]);
        dead.at[i]->flags = 0;
    }

    for_n(i, 0, to_free.len) {
        fe_inst_free(f, to_free.at[i]);
    }

    fe_wl_destroy(&wl);
    fe_wl_destroy(&dead);
    fe_wl_destroy(&to_free);
}
