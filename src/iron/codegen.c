#include "iron/iron.h"

void fe_vrbuf_init(FeVRegBuffer* buf, usize cap) {
    buf->len = 0;
    buf->cap = cap;
    buf->at = fe_malloc(cap * sizeof(buf->at[0]));
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
    def->vr_out = vr;
    return vr;
}

FeVirtualReg* fe_vreg(FeVRegBuffer* buf, FeVReg vr) {
    if (vr == FE_VREG_NONE) return NULL;
    return &buf->at[vr];
}

typedef struct {
    FeInstChain to;
    FeInst* from;
} InstPair;

void fe_isel(FeFunction* f) {
    fe_inst_update_uses(f);

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
            FeInstChain sel = fe_xr_isel(f, block, inst);
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
            FeInst** inputs = fe_inst_list_inputs(inst, &inputs_len);
            for_n(i, 0, inputs_len) {
                if (inputs[i]->flags == FE_ISEL_GENERATED) continue;

                inputs[i] = isel_map[inputs[i]->flags].to.end;
            }
        }
    }

    for_n(i, START, inst_count) {
        if (isel_map[i].from != isel_map[i].to.end) {
            fe_inst_free(f, isel_map[i].from);
        }
    }

    fe_free(isel_map);

    fe_xr_opt(f);
    fe_opt_tdce(f);

    // create virtual registers for instructions that dont have them yet
    for_blocks(block, f) {
        for_inst(inst, block) {
            if ((inst->ty != FE_TY_VOID || inst->ty != FE_TY_TUPLE) && inst->vr_out == FE_VREG_NONE) {
                // TODO choose register class based on architecture and type
                inst->vr_out = fe_vreg_new(f->vregs, inst, block, XR_REGCLASS_REG);
            }
        }
    }

    printf("isel complete\n");

    fe_regalloc_linear_scan(f);

    printf("regalloc complete\n");
}

char* fe_reg_name(FeArch arch, u8 class, u16 real) {
    switch (arch) {
    case FE_ARCH_XR17032:
        return fe_xr_reg_name(real);
    }
    return "?unknownarch";
}