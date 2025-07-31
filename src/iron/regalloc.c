#include "iron/iron.h"

bool is_canon_def(FeVirtualReg* inst_out, FeInst* inst) {
    // upsilons are a kind of fake definition that get created during codegen as inputs for phi nodes.
    return (inst->kind == FE__MACH_UPSILON || inst_out->def == inst);
}

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

static void calculate_liveness(FeFunc* f) {
    const FeTarget* t = f->mod->target;

    // make sure cfg is updated.

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
            if (inst->vr_def == FE_VREG_NONE) continue;
            
            FeInst** inputs = inst->inputs;
            for_n(i, 0, inst->in_len) {
                FeInst* input = inputs[i];
                FeVirtualReg* vr = fe_vreg(f->vregs, input->vr_def);
                if (input->kind == FE__MACH_UPSILON || vr->def_block != block) {
                    add_live_in(block->live, input->vr_def);
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
            for_n(succ_i, 0, block->succ_len) {
                FeBlock* succ = block->succ[succ_i];
                for_n(i, 0, succ->live->in_len) {
                    FeVReg succ_live_in = succ->live->in[i];
                    // add it to block.out
                    changed |= add_live_out(block->live, succ_live_in);
                    // if not defined in this block, add it block.in
                    FeVirtualReg* succ_live_in_vr = fe_vreg(f->vregs, succ_live_in);
                    if (!succ_live_in_vr->is_phi_out && succ_live_in_vr->def_block != block) {
                        changed |= add_live_in(block->live, succ_live_in);
                    }
                }
            }
        }
    }
}

typedef struct {
    bool** reg_live;
} LiveSet;

LiveSet liveset_new(const FeTarget* target) {
    LiveSet lvset;
    lvset.reg_live = fe_malloc(sizeof(lvset.reg_live[0]) * target->num_regclasses);
    for_n(i, 0, target->num_regclasses) {
        usize regclass_size = sizeof(lvset.reg_live[0][0]) * target->regclass_lens[i];
        lvset.reg_live[i] = fe_malloc(regclass_size);
        memset(lvset.reg_live[i], 0, regclass_size);
    }
    return lvset;
}

void liveset_add(LiveSet* lvset, u8 regclass, u16 reg) {
    lvset->reg_live[regclass][reg] = true;
}

void liveset_remove(LiveSet* lvset, u8 regclass, u16 reg) {
    lvset->reg_live[regclass][reg] = false;
}

bool is_live(LiveSet* lvset, u8 regclass, u16 reg) {
    return lvset->reg_live[regclass][reg];
}

void fe_regalloc_linear_scan(FeFunc* f) {
    FeVRegBuffer* vbuf = f->vregs;
    const FeTarget* target = f->mod->target;
    calculate_liveness(f);


    // hints!
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (!fe_inst_has_trait(inst->kind, FE_TRAIT_REG_MOV_HINT)) {
                continue;
            }

            FeVirtualReg* inst_vr = fe_vreg(f->vregs, inst->vr_def);

            // hint input and output to each other
            FeInst* input = inst->inputs[0];
            FeVirtualReg* input_vr = fe_vreg(f->vregs, input->vr_def);

            inst_vr->hint = input->vr_def;
            input_vr->hint = inst->vr_def;
        }
    }

    LiveSet lvset = liveset_new(target);

    for_blocks(block, f) {
        // make everything in live_out live now
        for_n(i, 0, block->live->out_len) {
            FeVirtualReg* vr = fe_vreg(vbuf, block->live->out[i]);
            if (vr->real != FE_VREG_REAL_UNASSIGNED) {
                liveset_add(&lvset, vr->class, vr->real);
            }
        }

        for_inst_reverse(inst, block) {
            if (inst->kind == FE_PHI || inst->vr_def != FE_VREG_NONE) {
                // this instruction defines a virtual register
                FeVirtualReg* inst_out = fe_vreg(vbuf, inst->vr_def);
                
                // if this instruction is the "canonical" definition of the 
                // virtual register, or the instruction is an upsilon inst,
                // KILL IT TO DEATH
                if (is_canon_def(inst_out, inst) && inst_out->real != FE_VREG_REAL_UNASSIGNED) {
                    liveset_remove(&lvset, inst_out->class, inst_out->real);
                }
            }

            FeInst** inst_inputs = inst->inputs;
            for_n(i, 0, inst->in_len) {
                FeInst* input = inst_inputs[i];
                FeVirtualReg* input_vr = fe_vreg(vbuf, input->vr_def);

                // if this vreg has already been allocated, skip past it
                if (input_vr->real != FE_VREG_REAL_UNASSIGNED) {
                    continue;
                }

                // allocate this virtual register a real one!!
                
                // try to take a hint lmao
                u16 real = 0;
                // TODO only try to take hints AFTER the entire regalloc is done.
                // taking hints early can clobber pre-colored registers

                // if (input_vr->hint != FE_VREG_NONE) {
                //     FeVirtualReg* hint_vr = fe_vreg(vbuf, input_vr->hint);
                //     if (hint_vr->real != FE_VREG_REAL_UNASSIGNED && !is_live(lvset, hint_vr->class, hint_vr->real)) {
                //         real = hint_vr->real;
                //     }
                // }

                // iterate through all the real registers
                if (!real) for (; real < target->regclass_lens[input_vr->class]; ++real) {
                    // right now, only consider call-clobbered registers. no spilling rn.
                    if (target->reg_status(f->sig->cconv, input_vr->class, real) != FE_REG_CALL_CLOBBERED) {
                        continue;
                    }
                    // if this real register is already used, skip past it.
                    if (is_live(&lvset, input_vr->class, real)) {
                        continue;
                    }

                    // we've found a register we can use here!
                    break;
                }
                if (real >= target->regclass_lens[input_vr->class]) {
                    FE_CRASH("unable to allocate");
                }

                // assign real register to virtual register
                input_vr->real = real;
                liveset_add(&lvset, input_vr->class, input_vr->real);
            }
        }

        // unlive everything that is live_in
        for_n(i, 0, block->live->in_len) {
            FeVirtualReg* vr = fe_vreg(vbuf, block->live->in[i]);
            liveset_remove(&lvset, vr->class, vr->real);
        }
    }
}
