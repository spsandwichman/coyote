#include "common/util.h"
#include "iron/iron.h"

// various local transformations

// returns new value or nullptr if didnt work
static FeInst* try_load_elim(FeFunc* f, FeWorklist* wlist, FeInst* load) {
    /*
        %0 = ...
        %1 = store [%0] %ptr, %val
        %2 = load [%1] %ptr
        %3 = ... %2
    ->
        %0 = ...
        %1 = store [%0] %ptr, %val
        %3 = ... %val
    */

    FeInst* dependent_store = load->inputs[0];
    // dependent operation is not a store
    if (dependent_store->kind != FE_STORE) {
        return nullptr;
    }
    // pointers dont match
    if (dependent_store->inputs[1] != load->inputs[1]) {
        return nullptr;
    }

    FeInst* new_val = dependent_store->inputs[2];

    fe_replace_uses(f, load, new_val);
    fe_inst_destroy(f, load);

    // add the store and its uses to the worklist
    fe_wl_push(wlist, dependent_store);
    for_n(i, 0, dependent_store->use_len) {
        fe_wl_push(wlist, FE_USE_PTR(dependent_store->uses[i]));
    }

    return new_val;
}

static FeInst* try_store_elim(FeFunc* f, FeWorklist* wlist, FeInst* store) {
    /*
        %0 = ...
        %1 = store [%0] %ptr, %val
        %2 = store [%1] %ptr, %val2
    ->
        %1 = store [%0] %ptr, %val2
    */


    FeInst* dependent_store = store->inputs[0];
    // dependent operation is not a store
    if (dependent_store->kind != FE_STORE) {
        return nullptr;
    }
    // pointers dont match
    if (dependent_store->inputs[1] != store->inputs[1]) {
        return nullptr;
    }

    // our store isnt the only thing using the dependent store
    if (dependent_store->use_len != 1) {
        return nullptr;
    }

    // use the dependent store's memory link
    fe_set_input(f, store, 0, dependent_store->inputs[0]);
    fe_inst_destroy(f, dependent_store);

    // add the store and its uses to the worklist
    fe_wl_push(wlist, store);
    for_n(i, 0, store->use_len) {
        fe_wl_push(wlist, FE_USE_PTR(store->uses[i]));
    }

    return store;
}

void fe_opt_local(FeFunc* f) {
    FeWorklist wlist;
    fe_wl_init(&wlist);

    for_blocks(block, f) {
        for_inst(inst, block) {
            fe_wl_push(&wlist, inst);
        }
    }

    while (wlist.len != 0) {
        FeInst* inst = fe_wl_pop(&wlist);

        switch (inst->kind) {
        case FE_LOAD: {
            try_load_elim(f, &wlist, inst);
        } break;
        case FE_STORE: {
            try_store_elim(f, &wlist, inst);
        } break;
        }
    }

    fe_wl_destroy(&wlist);
}
