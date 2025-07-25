#include "iron/iron.h"

void fe_vrbuf_init(FeVRegBuffer* buf, usize cap) {
    if (cap < 2) cap = 2;
    buf->len = 0;
    buf->cap = cap;
    buf->at = fe_malloc(cap * sizeof(buf->at[0]));
}

void fe_vrbuf_clear(FeVRegBuffer* buf) {
    // memset(buf->at, 0, buf->len * sizeof(buf->at[0]));
    buf->len = 0;
}

void fe_vrbuf_destroy(FeVRegBuffer* buf) {
    fe_free(buf->at);
    *buf = (FeVRegBuffer){};
}

FeVReg fe_vreg_new(FeVRegBuffer* buf, FeInst* def, FeBlock* def_block, u8 class) {
    if (buf->len == buf->cap) {
        buf->cap += buf->cap >> 1;
        buf->at = fe_realloc(buf->at, buf->cap * sizeof(buf->at[0]));
    }
    FeVReg vr = buf->len++;
    buf->at[vr].class = class;
    buf->at[vr].def = def;
    buf->at[vr].def_block = def_block;
    buf->at[vr].real = FE_VREG_REAL_UNASSIGNED;
    buf->at[vr].hint = FE_VREG_NONE;
    buf->at[vr].is_phi_out = def->kind == FE_PHI;
    def->vr_out = vr;
    return vr;
}

FeVirtualReg* fe_vreg(FeVRegBuffer* buf, FeVReg vr) {
    if (vr == FE_VREG_NONE) return nullptr;
    return &buf->at[vr];
}

static void insert_upsilon(FeFunc* f) {
    // for_blocks(block, f) {
    //     for_inst(inst, block) {
    //         if (inst->kind != FE_PHI) continue;

    //         usize len = fe_extra(inst, FeInstPhi)->len;
    //         FeInst** srcs = fe_extra(inst, FeInstPhi)->vals;
    //         FeBlock** blocks = fe_extra(inst, FeInstPhi)->blocks;

    //         // add upsilon nodes
    //         for_n(i, 0, len) {
    //             FeInst* upsilon = fe_inst_unop(f, inst->ty, FE__MACH_UPSILON, srcs[i]);
    //             fe_insert_before(blocks[i]->bookend->prev, upsilon);
    //             srcs[i] = upsilon;
    //         }
    //     }
    // }
    FE_ASSERT(false);
}

typedef struct {
    FeInstChain to;
    FeInst* from;
} InstPair;

void fe_codegen(FeFunc* f) {

    insert_upsilon(f);

    const FeTarget* target = f->mod->target;

    fe_stack_calculate_size(f);


    usize START = 1;
    // assign each instruction an index starting from 1.
    usize inst_count = START;
    for_blocks(block, f) {
        for_inst(inst, block) {
            inst->flags = inst_count++;
        }
    }

    InstPair* isel_map = fe_malloc(sizeof(*isel_map) * inst_count);
    memset(isel_map, 0, sizeof(*isel_map) * inst_count);

    for_blocks(block, f) {
        for_inst(inst, block) {
            FeInstChain sel = target->isel(f, block, inst);
            isel_map[inst->flags].from = inst;
            isel_map[inst->flags].to = sel;
        }
    }

    // replace instructions with selected instructions
    for_n(i, START, inst_count) {
        fe_chain_replace_pos(isel_map[i].from, isel_map[i].to);
    }

    // replace inputs to all selected instructions
    for_blocks(block, f) {
        for_inst(inst, block) {
            usize inputs_len = 0;
            FeInst** inputs = inst->inputs;
            for_n(i, 0, inputs_len) {
                FeInst* new_input = isel_map[inputs[i]->flags].to.end;
                if (new_input != nullptr) {
                    inputs[i] = new_input;
                }
            }
        }
    }

    for_n(i, START, inst_count) {
        if (isel_map[i].from != isel_map[i].to.end) {
            fe_inst_destroy(f, isel_map[i].from);
        }
    }

    fe_free(isel_map);

    target->pre_regalloc_opt(f);
    fe_opt_local(f);

    // create virtual registers for instructions that dont have them yet
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->kind == FE__MACH_UPSILON) continue;
            if ((inst->ty != FE_TY_VOID && inst->ty != FE_TY_TUPLE) && inst->vr_out == FE_VREG_NONE) {
                // TODO choose register class based on architecture and type
                inst->vr_out = fe_vreg_new(f->vregs, inst, block, target->choose_regclass(inst->kind, inst->ty));
            }
        }
    }
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->kind != FE_PHI) continue;

            FE_ASSERT(false);

            // usize len = fe_extra(inst, FeInstPhi)->len;
            // FeInst** srcs = fe_extra(inst, FeInstPhi)->vals;

            // // add upsilon nodes
            // for_n(i, 0, len) {
            //     FeInst* upsilon = srcs[i];
            //     upsilon->vr_out = inst->vr_out;
            // }
        }
    }
    
    fe_regalloc_linear_scan(f);

    f->mod->target->final_touchups(f);
}

void fe_emit_asm(FeDataBuffer* db, FeModule* m) {
    m->target->emit_asm(db, m);
}
