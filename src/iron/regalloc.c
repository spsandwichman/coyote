#include "iron/iron.h"

static bool add_live_in(FeBlockLiveness* lv, FeVReg vr) {
    // check to see if its already in the live-in_set.
    for_n(i, 0, lv->in_len) {
        if (lv->in[i] == vr) {
            return false;
        }
    }
    // vr is not in live-in. add it.
    if (lv->in_len == lv->in_cap) {
        lv->in_cap += lv->in_cap >> 1;
        lv->in = fe_realloc(lv->in, sizeof(lv->in[0]) * lv->in_cap);
    }
    lv->in[lv->in_len++] = vr;
    return true;
}

static bool add_live_out(FeBlockLiveness* lv, FeVReg vr) {
    // check to see if its already in the live-out set.
    for_n(i, 0, lv->out_len) {
        if (lv->out[i] == vr) {
            return false;
        }
    }
    // vr is not in live-out. add it.
    if (lv->out_len == lv->out_cap) {
        lv->out_cap += lv->out_cap >> 1;
        lv->out = fe_realloc(lv->out, sizeof(lv->out[0]) * lv->out_cap);
    }
    lv->out[lv->out_len++] = vr;
    return true;
}

static void calculate_liveness(FeFunction* f) {
    FeTarget* t = f->mod->target;

    // make sure cfg is updated.
    fe_calculate_cfg(f);

    // give every basic block a liveness chunk.
    for_blocks(block, f) {
        FeBlockLiveness* lv = fe_malloc(sizeof(FeBlockLiveness));
        memset(lv, 0, sizeof(FeBlockLiveness));
        lv->block = block;
        block->live = lv;

        // initialize live-in/live-out vectors
        lv->in_cap = 16;
        lv->out_cap = 16;
        lv->in = fe_malloc(sizeof(lv->in[0]) * lv->in_cap);
        lv->out = fe_malloc(sizeof(lv->out[0]) * lv->out_cap);
    }
    
    // initialize simple live-ins
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->vr_out == FE_VREG_NONE) continue;
            
            usize inputs_len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &inputs_len);
            for_n(i, 0, inputs_len) {
                FeInst* input = inputs[i];
                if (fe_vreg(f->vregs, input->vr_out)->def_block != block) {
                    add_live_in(block->live, input->vr_out);
                }
            }
        }
    }

    // iterate over the blocks, refining liveness until everything is settled
    bool changed = true;
    while (changed) {
        changed = false;
        // block.live_out = block.live_out U successor0.live_in U successor1.live_in ...
        for_blocks(block, f) {
            for_n(succ_i, 0, block->cfg_node->out_len) {
                FeBlock* succ = fe_cfgn_out(block->cfg_node, succ_i)->block;
                for_n(i, 0, succ->live->in_len) {
                    FeVReg succ_live_in = succ->live->in[i];
                    // add it to block.out
                    changed |= add_live_out(block->live, succ_live_in);
                    // if not defined in this block, add it block.in
                    if (fe_vreg(f->vregs, succ_live_in)->def_block != block) {
                        changed |= add_live_in(block->live, succ_live_in);
                    }
                }
            }
        }
    }
}

void fe_regalloc_linear_scan(FeFunction* f) {
    FeTarget* target = f->mod->target;
    calculate_liveness(f);

    // hints!
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (!fe_inst_has_trait(inst->kind, FE_TRAIT_REG_MOV_HINT)) {
                continue;
            }

            FeVirtualReg* inst_vr = fe_vreg(f->vregs, inst->vr_out);

            // hint input and output to each other
            FeInst* input = fe_extra_T(inst, FeInstUnop)->un;
            FeVirtualReg* input_vr = fe_vreg(f->vregs, input->vr_out);

            inst_vr->hint = input->vr_out;
            input_vr->hint = inst->vr_out;
        }
    }

    bool* vr_live_now = fe_malloc(f->vregs->len);
    memset(vr_live_now, 0, f->vregs->len);

    // TODO change assumption that we're on XR
    bool** real_live_now = fe_malloc(sizeof(real_live_now[0]) * (target->max_regclass + 1));
    // memset(real_live_now, 0, XR_REG__COUNT);
    for_n(i, 0, target->max_regclass + 1) {
        real_live_now[i] = fe_malloc(sizeof(real_live_now[0][0]) * target->regclass_lens[i]);
    }

    for_blocks(block, f) {
        // initialize is_live_now from block.live_out
        for_n(i, 0, block->live->out_len) {
            FeVReg out = block->live->out[i];
            vr_live_now[out] = true;
        }

        for_inst_reverse(inst, block) {
            FeVReg current = inst->vr_out;
            FeVirtualReg* current_vr = fe_vreg(f->vregs, current);

            if (current != FE_VREG_NONE && current_vr->real == FE_VREG_REAL_UNASSIGNED) {
                // see if we can allocate some shit
                // if we have a hint, try to take it
                FeVirtualReg* hint_vr = fe_vreg(f->vregs, current_vr->hint);
                if (hint_vr != NULL && hint_vr->real != FE_VREG_REAL_UNASSIGNED && !real_live_now[hint_vr->class][hint_vr->real]) {
                    current_vr->real = hint_vr->real;
                } else {
                    FeRegclass regclass = current_vr->class; 
                    // try to allocate output register to a call_clobbered register first
                    u16 unused_real_reg = 0;
                    for (; unused_real_reg != target->regclass_lens[regclass]; unused_real_reg++) {
                        if (target->reg_status(f->sig->cconv, regclass, unused_real_reg) != FE_REG_CALL_CLOBBERED) continue;
                        // skip if its currently live.
                        if (real_live_now[regclass][unused_real_reg]) continue;
                        break;
                    }
                    if (unused_real_reg != target->regclass_lens[regclass]) {
                        current_vr->real = unused_real_reg;
                    } else {
                        fe_runtime_crash("regalloc wuhhhhhhh");
                    }
                }
                
            }

            // kill output if its canonical definition is the current inst
            // (for supporting two-address instructions)
            if (current != FE_VREG_NONE && inst == current_vr->def) {
                vr_live_now[current] = false;
                if (current_vr->real != FE_VREG_REAL_UNASSIGNED) {
                    real_live_now[current_vr->class][current_vr->real] = false;
                }
            }

            usize inputs_len;
            FeInst** inputs = fe_inst_list_inputs(target, inst, &inputs_len);
            // make inputs live
            for_n(i, 0, inputs_len) {
                FeInst* inst_input = inputs[i];
                FeVReg input = inst_input->vr_out;
                FeVirtualReg* input_vr = fe_vreg(f->vregs, input);

                vr_live_now[input] = true;

                if (input_vr->real != FE_VREG_REAL_UNASSIGNED) {
                    real_live_now[input_vr->class][input_vr->real] = true;
                }
            }

        }

        // uninitialize is_live_now from block.live_in
        for_n(i, 0, block->live->in_len) {
            FeVReg in = block->live->in[i];
            vr_live_now[in] = false;
        }
        memset(vr_live_now, 0, f->vregs->len);
        // memset(real_live_now, 0, XR_REG__COUNT);
    }
}