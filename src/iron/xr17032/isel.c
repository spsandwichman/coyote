#include "iron/iron.h"
#include "xr.h"

static FeInst* xr_inst(FeFunc* f, XrInstKind kind, usize input_len, usize extra_size) {
    FeInst* inst = fe_inst_new(f, input_len, extra_size);
    inst->kind = kind;
    inst->vr_def = 0;
    return inst;
}

static FeInst* mach_reg(FeFunc* f, FeBlock* b, XrGpr real) {
    FeInst* src_reg = fe_inst_new(f, 0, 0);
    src_reg->kind = FE__MACH_REG;
    src_reg->vr_def = fe_vreg_new(f->vregs, src_reg, b, XR_REGCLASS_GPR);
    fe_vreg(f->vregs, src_reg->vr_def)->real = real;
    return src_reg;
}

static bool is_const_u16(FeInst* inst) {
    if (inst->kind != FE_CONST) {
        return false;
    }
    u64 val = fe_extra(inst, FeInstConst)->val;
    return (val & 0xFFFF) == val;
}

static u64 const_val(FeInst* inst) {
    return fe_extra(inst, FeInstConst)->val;
}

static FeInstChain get_parameter(FeFunc* f, FeBlock* entry, usize index) {
    switch (f->sig->cconv) {
    case FE_CCONV_ANY:
    case FE_CCONV_JACKAL:
        break;
    default:
        FE_CRASH("unsupported cconv");
    }

    FeTy param_ty = f->sig->params[index].ty;

    if (index < 4) {
        // integer argument through registers.
        static u16 arg_regs[4] = {XR_GPR_A0, XR_GPR_A1, XR_GPR_A2, XR_GPR_A3};
        
        // create move from register
        FeInst* reg = mach_reg(f, entry, arg_regs[index]);
        reg->ty = param_ty;
        FeInst* mov = fe_inst_unop(f, param_ty, FE_MOV, reg);

        FeInstChain chain = fe_chain_new(reg);
        chain = fe_chain_append_end(chain, mov);
        return chain;
    } else {
        FE_CRASH("stack args so far unsupported");
    }
}

static FeInstChain store_returnval(FeFunc* f, FeBlock* exit, usize index, FeInst* value) {
    switch (f->sig->cconv) {
    case FE_CCONV_ANY:
    case FE_CCONV_JACKAL:
        break;
    default:
        FE_CRASH("unsupported cconv");
    }

    FeTy param_ty = f->sig->params[index].ty;

    if (index < 4) {
        // integer argument through registers.
        static u16 ret_regs[4] = {XR_GPR_A3, XR_GPR_A2, XR_GPR_A1, XR_GPR_A0};
        
        // create move to register
        FeInst* mov = fe_inst_unop(f, param_ty, FE_MOV, value);
        mov->vr_def = fe_vreg_new(f->vregs, mov, exit, XR_REGCLASS_GPR);
        fe_vreg(f->vregs, mov->vr_def)->real = ret_regs[index];

        FeInstChain chain = fe_chain_new(mov);
        return chain;
    } else {
        FE_CRASH("stack returns so far unsupported");
    }
}

FeInstChain fe_xr_isel(FeFunc* f, FeBlock* block, FeInst* inst) {
    switch (inst->kind) {
    case FE__ROOT:
        return FE_EMPTY_CHAIN;
    case FE_PROJ:
        if (inst->inputs[0]->kind == FE__ROOT) {
            // FE_CRASH("select parameter");

            // figure out what parameter we're looking at.
            usize index = fe_extra(inst, FeInstProj)->index;
            return get_parameter(f, block, index);
        }
        FE_CRASH("unknown proj selection");
    case FE_IADD: {
        if (is_const_u16(inst->inputs[1])) {
            FeInst* sel = xr_inst(f, XR_ADDI, 1, sizeof(XrInstImm));
            fe_set_input(f, sel, 0, inst->inputs[0]);
            fe_extra(sel, XrInstImm)->imm = const_val(inst->inputs[1]);
            return fe_chain_new(sel);
        }
        FeInst* sel = xr_inst(f, XR_ADD, 2, sizeof(XrInstImm));
        fe_set_input(f, sel, 0, inst->inputs[0]);
        fe_set_input(f, sel, 0, inst->inputs[1]);
        return fe_chain_new(sel);
    }
    case FE_CONST: {
        if (is_const_u16(inst)) {
            // create addi with zero
            FeInst* zero = mach_reg(f, block, XR_GPR_ZERO);
            FeInst* addi = xr_inst(f, XR_ADDI, 1, sizeof(XrInstImm));
            fe_extra(addi, XrInstImm)->imm = const_val(inst);
            fe_set_input(f, addi, 0, zero);

            FeInstChain chain = fe_chain_new(zero);
            chain = fe_chain_append_end(chain, addi);
            return chain;
        }
        FE_CRASH("too big");
    }
    // case FE_RETURN: {

    // }
    default:
        FE_CRASH("cannot select from inst %s (%d)", fe_inst_name(f->mod->target, inst->kind), inst->kind);
    }
}