#include "iron.h"
#include "iron/iron.h"

#include "short_traits.h"

// verify - IR correctness verification pass
// only built to work on generic insts

static void inst_local_check(FeFunc* f, FeInst* inst) {
    const FeTarget* target = f->mod->target;

    const char* inst_name = fe_inst_name(target, inst->kind);
    if (!inst_name) {
        fe_runtime_crash("inst kind %d is not recognized", inst->kind);
    }

    if (fe_inst_has_trait(inst->kind, TERM) && inst->next->kind != FE_BOOKEND) {
        fe_runtime_crash("inst %s is a terminator but is not last in block", inst_name);
    }
    if (!fe_inst_has_trait(inst->kind, TERM) && inst->next->kind == FE_BOOKEND) {
        fe_runtime_crash("inst %s is not a terminator but is last in block", inst_name);
    }

    usize inputs_len = 0;
    FeInst** inputs = fe_inst_list_inputs(target, inst, &inputs_len);
    for_n(i, 0, inputs_len) {
        FeInst* input = inputs[i];
        if (fe_inst_has_trait(inst->kind, SAME_INS)) {
            if (input[i].ty != input[0].ty) {
                fe_runtime_crash("inst %s has trait SAME_INPUT_TYS but input[%zu].ty != %s", inst_name, i, fe_ty_name(input[0].ty));
            }
        }
    }
}

void fe_verify_func(FeFunc* f) {
    fe_runtime_crash("ugh");
}