#include "xr.h"

static FeInst* xr_inst(FeFunc* f, XrInstKind kind, usize input_len, usize extra_size) {
    FeInst* inst = fe_inst_new(f, input_len, extra_size);
    inst->kind = kind;

    inst->vr_def = 0;
}

FeInstChain fe_xr_isel(FeFunc* f, FeBlock* block, FeInst* inst) {
    switch (inst->kind) {
    case FE__ROOT:
        return FE_EMPTY_CHAIN;
    case FE_PROJ:
        if (inst->inputs[0]->kind == FE__ROOT) {
            // select parameter
        }
    }
}