#include "iron/iron.h"

static bool is_const_zero(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val == 0;
}

static bool can_const_u16(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val <= UINT16_MAX;
}

static bool can_subi_const_u16(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val >= 0xFFFF0001ull;
}

static u16 as_subi_const_u16(FeInst* inst) {
    // imm = -x
    return (u16)-fe_extra_T(inst, FeInstConst)->val;
}

static u16 as_u16(FeInst* inst) {
    return (u16)fe_extra_T(inst, FeInstConst)->val;
}

static u16 as_hi_u16(FeInst* inst) {
    return (u16)(fe_extra_T(inst, FeInstConst)->val >> 16);
}

static FeInst* mach_reg(FeFunction* f, FeBlock* block, u16 real_reg) {
    FeInst* zero = fe_ipool_alloc(f->ipool, 0);
    zero->kind = FE_MACH_REG;
    zero->ty = FE_TY_I32;
    FeVReg zero_reg = fe_vreg_new(f->vregs, zero, block, XR_REGCLASS_REG);
    fe_vreg(f->vregs, zero_reg)->real = real_reg;
    return zero;
}

static FeInst* create_mach(FeFunction* f, FeInstKind kind, FeTy ty, usize size) {
    FeInst* i = fe_ipool_alloc(f->ipool, size);
    i->kind = kind; 
    i->ty = ty;
    i->flags = FE_ISEL_GENERATED;
    // i->vr_out = (ty == FE_TY_TUPLE || ty == FE_TY_VOID) ? FE_VREG_NONE : fe_vreg_new(f->vregs, i, XR_REGCLASS_REG);
    i->vr_out = FE_VREG_NONE;
    return i;
}

static FeInst* mov_volatile(FeFunction* f, FeInst* source) {
    FeInst* i = fe_inst_unop(f, FE_TY_I32, FE_MOV_VOLATILE, source);
    i->flags = FE_ISEL_GENERATED;
    // i->vr_out = fe_vreg_new(f->vregs, i, XR_REGCLASS_REG);
    i->vr_out = FE_VREG_NONE;
    return i;
}

static void preassign(FeFunction* f, FeInst* inst, FeBlock* block, u16 real_reg) {
    if (inst->vr_out == FE_VREG_NONE) {
        inst->vr_out = fe_vreg_new(f->vregs, inst, block, XR_REGCLASS_REG);
    }
    fe_vreg(f->vregs, inst->vr_out)->real = real_reg;
}

// static void set_hint(FeFunction* f, FeInst* inst, FeBlock* block, u16 real_reg) {
//     if (inst->vr_out == FE_VREG_NONE) {
//         inst->vr_out = fe_vreg_new(f->vregs, inst, block, XR_REGCLASS_REG);
//     }
//     fe_vreg(f->vregs, inst->vr_out)->hint = real_reg;
// }

FeInstChain fe_xr_isel(FeFunction* f, FeBlock* block, FeInst* inst) {
    void* extra = fe_extra(inst);
    FeInstBinop* binop = extra;

    switch (inst->kind) {
    case FE_PARAM: {
        // params become a mach_reg and mov_volatile

        u16 real_param_reg = XR_REG_ZERO;
        switch (fe_extra_T(inst, FeInstParam)->index) {
        case 0: real_param_reg = XR_REG_A0; break;
        case 1: real_param_reg = XR_REG_A1; break;
        case 2: real_param_reg = XR_REG_A2; break;
        case 3: real_param_reg = XR_REG_A3; break;
        default:
            fe_runtime_crash("pass on stack later LOL");
        }

        FeInst* param_reg = mach_reg(f, block, real_param_reg);
        FeInst* mov = mov_volatile(f, param_reg);
        // set_hint(f, mov, block, real_param_reg);
        FeInstChain chain = fe_new_chain(param_reg);
        chain = fe_chain_append_end(chain, mov);
        return chain;
    }
    case FE_CONST:
        if (can_const_u16(inst)) {
            // emit as addi zero.
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);
            FeInst* addi = create_mach(f, FE_XR_ADDI, FE_TY_I32, sizeof(FeXrRegImm16));
            fe_extra_T(addi, FeXrRegImm16)->reg = zero;
            fe_extra_T(addi, FeXrRegImm16)->num = as_u16(inst);
            FeInstChain chain = fe_new_chain(addi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } if (can_subi_const_u16(inst)) {
            // emit as subi zero
            FeInst* subi = create_mach(f, FE_XR_SUBI, FE_TY_I32, sizeof(FeXrRegImm16));
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);
            fe_extra_T(subi, FeXrRegImm16)->reg = zero;
            fe_extra_T(subi, FeXrRegImm16)->num = as_subi_const_u16(inst);
            FeInstChain chain = fe_new_chain(subi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } else {
            // lui + addi pair
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);

            FeInst* lui = create_mach(f, FE_XR_LUI, FE_TY_I32, sizeof(FeXrRegImm16));

            fe_extra_T(lui, FeXrRegImm16)->reg = zero;
            fe_extra_T(lui, FeXrRegImm16)->num = as_hi_u16(inst);

            FeInst* addi = create_mach(f, FE_XR_ADDI, FE_TY_I32, sizeof(FeXrRegImm16));
            fe_extra_T(addi, FeXrRegImm16)->reg = lui;
            fe_extra_T(addi, FeXrRegImm16)->num = as_u16(inst);

            FeInstChain chain = fe_new_chain(zero);
            chain = fe_chain_append_end(chain, lui);
            chain = fe_chain_append_end(chain, addi);
            return chain;
        }
        return fe_new_chain(inst);
    case FE_IADD:
        if (can_const_u16(binop->rhs)) {
            FeInst* addi = create_mach(f, FE_XR_ADDI, FE_TY_I32, sizeof(FeXrRegImm16));
            fe_extra_T(addi, FeXrRegImm16)->reg = binop->lhs;
            fe_extra_T(addi, FeXrRegImm16)->num = as_u16(binop->rhs);
            return fe_new_chain(addi);
        } else {
            FeInst* add = create_mach(f, FE_XR_ADD, FE_TY_I32, sizeof(FeXrRegReg));
            fe_extra_T(add, FeXrRegReg)->lhs = binop->lhs;
            fe_extra_T(add, FeXrRegReg)->rhs = binop->rhs;
            return fe_new_chain(add);
        }
        break;
    case FE_ISUB:
        if (can_const_u16(binop->rhs)) {
            FeInst* sub = create_mach(f, FE_XR_SUBI, FE_TY_I32, sizeof(FeXrRegImm16));
            fe_extra_T(sub, FeXrRegImm16)->reg = binop->lhs;
            fe_extra_T(sub, FeXrRegImm16)->num = as_u16(binop->rhs);
            return fe_new_chain(sub);
        } else {
            FeInst* sub = create_mach(f, FE_XR_SUB, FE_TY_I32, sizeof(FeXrRegReg));
            fe_extra_T(sub, FeXrRegReg)->lhs = binop->lhs;
            fe_extra_T(sub, FeXrRegReg)->rhs = binop->rhs;
            return fe_new_chain(sub);
        }
    case FE_IMUL: {
        FeInst* mul = create_mach(f, FE_XR_MUL, FE_TY_I32, sizeof(FeXrRegReg));
        fe_extra_T(mul, FeXrRegReg)->lhs = binop->lhs;
        fe_extra_T(mul, FeXrRegReg)->rhs = binop->rhs;
        return fe_new_chain(mul);
    }
    case FE_BRANCH: {
        if (fe_extra_T(inst, FeInstBranch)->cond->kind == FE_EQ) {
            FeInstBinop* cmp_eq = fe_extra(fe_extra_T(inst, FeInstBranch)->cond);

            FeInst* beq = create_mach(f, FE_XR_BEQ, FE_TY_VOID, sizeof(FeXrRegBranch));
            FeInstChain chain = fe_new_chain(beq);

            FeInst* result;
            if (is_const_zero(cmp_eq->rhs)) {
                //  xr.beq %0, 1:, 2:
                result = cmp_eq->lhs;
            } else if (can_const_u16(cmp_eq->rhs)) {
                //  %2: i32 = xr.subi %0, CONST16
                //  xr.beq %2, 1:, 2:
                result = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
                result->kind = FE_XR_SUBI; result->ty = FE_TY_I32;
                fe_extra_T(result, FeXrRegImm16)->reg = cmp_eq->lhs;
                fe_extra_T(result, FeXrRegImm16)->num = as_u16(cmp_eq->rhs);
                chain = fe_chain_append_begin(chain, result);
            } else {
                //  %2: i32 = xr.sub %0, %1
                //  xr.beq %2, 1:, 2:
                result = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
                result->kind = FE_XR_SUB; result->ty = FE_TY_I32;
                fe_extra_T(result, FeXrRegReg)->lhs = cmp_eq->lhs;
                fe_extra_T(result, FeXrRegReg)->rhs = cmp_eq->rhs;
                chain = fe_chain_append_begin(chain, result);
            }

            fe_extra_T(beq, FeXrRegBranch)->reg = result;
            fe_extra_T(beq, FeXrRegBranch)->dest = fe_extra_T(inst, FeInstBranch)->if_true;
            fe_extra_T(beq, FeXrRegBranch)->_else = fe_extra_T(inst, FeInstBranch)->if_false;

            return chain;
        }
        return fe_new_chain(inst);
    }
    case FE_RETURN: {
        FeInstReturn* inst_ret = extra;
        FeInst* ret = fe_ipool_alloc(f->ipool, 0);
        ret->kind = FE_XR_RET;
        ret->ty = FE_TY_VOID;
        FeInstChain chain = fe_new_chain(ret);
        for_n(i, 0, inst_ret->len) {
            FeInst* arg = fe_return_arg(inst, i);
            FeInst* mov = mov_volatile(f, arg);
            
            u16 real_return_reg = XR_REG_ZERO;
            switch (i) {
            case 0: real_return_reg = XR_REG_A3; break;
            case 1: real_return_reg = XR_REG_A2; break;
            case 2: real_return_reg = XR_REG_A1; break;
            case 3: real_return_reg = XR_REG_A0; break;
            default:
                fe_runtime_crash("pass on stack later LOL");
            }

            preassign(f, mov, block, real_return_reg);
            chain = fe_chain_append_begin(chain, mov);
        }
        return chain;
    }
    }
    return fe_new_chain(inst);
}