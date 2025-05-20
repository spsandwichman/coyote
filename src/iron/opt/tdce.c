#include <iron/iron.h>

// TDCE - trivial dead code elimination

typedef struct {
    FeInst** at;
    u32 len;
    u32 cap;
} Worklist;

static void init_worklist(Worklist* wl) {
    wl->cap = 256;
    wl->len = 0;
    wl->at = fe_malloc(sizeof(wl->at[0]) * wl->cap);
}

static void worklist_push(Worklist* wl, FeInst* inst) {
    if (wl->len == wl->cap) {
        wl->cap += wl->cap >> 1;
        wl->at = fe_realloc(wl->at, sizeof(wl->at[0]) * wl->cap);
    }
    wl->at[wl->len++] = inst;
}

static FeInst* worklist_pop(Worklist* wl) {
    return wl->at[--wl->len];
}

static void worklist_destroy(Worklist* wl) {
    fe_free(wl->at);
    *wl = (Worklist){0};
}

#define TDCE_DEAD_LMAO 0xDEADDEAD

void fe_opt_tdce(FeFunc* f) {
    const FeTarget* t = f->mod->target;

    // TODO fuck all these worklists into OUTERED SPACE
    Worklist wl;
    Worklist dead;
    Worklist to_free;
    init_worklist(&wl);
    init_worklist(&dead);
    init_worklist(&to_free);

    // update every instruction's use count
    fe_inst_calculate_uses(f);

    for_blocks(block, f) {
        for_inst(inst, block) {
            worklist_push(&wl, inst);
        }
    }

    while (wl.len != 0) {
        FeInst* inst = worklist_pop(&wl);
        // if (inst->kind == FE_PARAM) {
            // continue;
        // }

        if (inst->use_count == 0 && !fe_inst_has_trait(inst->kind, FE_TRAIT_VOLATILE)) {
            // get rid of it!!!!!!!!!!!!
            worklist_push(&dead, inst);
            inst->flags = TDCE_DEAD_LMAO;

            // decrement all input use counts
            usize len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &len);
            for_n (i, 0, len) {
                if (inputs[i]->flags == TDCE_DEAD_LMAO) {
                    continue;
                }
                inputs[i]->use_count--;
                if (inputs[i]->use_count == 0) {
                    worklist_push(&wl, inputs[i]);
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
        worklist_push(&to_free, dead.at[i]);
        dead.at[i]->flags = 0;
    }

    for_n(i, 0, to_free.len) {
        fe_inst_free(f, to_free.at[i]);
    }

    worklist_destroy(&wl);
    worklist_destroy(&dead);
    worklist_destroy(&to_free);
}
