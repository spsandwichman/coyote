#include "iron/iron.h"
#include "iron/xr17032/xr.h"

// target construction.
const FeTarget* fe_make_target(FeArch arch, FeSystem system) {
    FeTarget* t = fe_malloc(sizeof(*t));
    memset(t, 0, sizeof(*t));
    t->arch = arch;
    t->system = system;

    switch (arch) {
    case FE_ARCH_XR17032:
        t->ir_print_args = xr_print_args;
        t->inst_name = xr_inst_name;
        t->list_inputs = xr_list_inputs;
        t->list_targets = xr_term_list_targets;
        t->isel = xr_isel;
        t->reg_name = xr_reg_name;
        t->reg_status = xr_reg_status;
        t->num_regclasses = XR_REGCLASS_REG + 1;
        t->regclass_lens = xr_regclass_lens;
        t->emit_asm = xr_emit_assembly;
        t->choose_regclass = xr_choose_regclass;
        t->pre_regalloc_opt = xr_pre_regalloc_opt;
        t->final_touchups = xr_final_touchups;
        t->stack_pointer_align = 4;

        fe__load_extra_size_table(FE__XR_INST_BEGIN, xr_size_table, FE__XR_INST_END - FE__XR_INST_BEGIN);
        fe__load_trait_table(FE__XR_INST_BEGIN, xr_trait_table, FE__XR_INST_END - FE__XR_INST_BEGIN);
        break;
    default:
        FE_CRASH("arch unsupported");
    }

    switch (system) {
    case FE_SYSTEM_FREESTANDING:
        break;
    default:
        FE_CRASH("system unsupported");
    }

    return t;
}
