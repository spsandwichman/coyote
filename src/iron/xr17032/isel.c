#include "iron/iron.h"

static bool can_const_u16(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val <= UINT16_MAX;
}

static u16 as_u16(FeInst* inst) {
    return (u16)fe_extra_T(inst, FeInstConst)->val;
}

static u16 as_hi_u16(FeInst* inst) {
    return (u16)(fe_extra_T(inst, FeInstConst)->val >> 16);
}


static FeInst* fake_zero(FeFunction* f) {
    FeInst* zero = fe_ipool_alloc(f->ipool, 0);
    zero->kind = FE_MACH_REG;
    zero->ty = FE_TY_I32;
    FeVReg zero_reg = fe_vreg_new(f->vregs, zero, XR_REGCLASS_REG);
    fe_vreg(f->vregs, zero_reg)->real = XR_REG_ZERO;
    return zero;
}

FeInstChain fe_xr_isel(FeFunction* f, FeInst* inst) {
    void* extra = fe_extra(inst);
    FeInstBinop* binop = extra;

    switch (inst->kind) {
    case FE_CONST:
        if (can_const_u16(inst)) {
            // emit as just an ADDI.
            FeInst* addi = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            addi->kind = FE_XR_ADDI; addi->ty = FE_TY_I32;
            FeInst* zero = fake_zero(f);
            fe_extra_T(addi, FeXrRegImm16)->val = zero;
            fe_extra_T(addi, FeXrRegImm16)->num = as_u16(inst);
            FeInstChain chain = fe_new_chain(addi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } else {
            // emit as an addi and an lui.
            FeInst* addi = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            addi->kind = FE_XR_ADDI; addi->ty = FE_TY_I32;
            FeInst* zero = fake_zero(f);
            fe_extra_T(addi, FeXrRegImm16)->val = zero;
            fe_extra_T(addi, FeXrRegImm16)->num = as_u16(inst);

            FeInst* lui = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            lui->kind = FE_XR_LUI; lui->ty = FE_TY_I32;

            fe_extra_T(lui, FeXrRegImm16)->val = addi;
            fe_extra_T(lui, FeXrRegImm16)->num = as_hi_u16(inst);

            FeInstChain chain = fe_new_chain(zero);
            chain = fe_chain_append_end(chain, addi);
            chain = fe_chain_append_end(chain, lui);
            return chain;
        }
        return fe_new_chain(inst);
    case FE_IADD:
        if (can_const_u16(binop->rhs)) {
            FeInst* add = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            add->kind = FE_XR_ADDI; add->ty = FE_TY_I32;
            fe_extra_T(add, FeXrRegImm16)->val = binop->lhs;
            fe_extra_T(add, FeXrRegImm16)->num = as_u16(binop->rhs);
            return fe_new_chain(add);
        } else {
            FeInst* add = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
            add->kind = FE_XR_ADD; add->ty = FE_TY_I32;
            fe_extra_T(add, FeXrRegReg)->lhs = binop->lhs;
            fe_extra_T(add, FeXrRegReg)->rhs = binop->rhs;
            return fe_new_chain(add);
        }
        break;
    case FE_ISUB:
        if (can_const_u16(binop->rhs)) {
            FeInst* sub = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            sub->kind = FE_XR_SUBI; sub->ty = FE_TY_I32;
            fe_extra_T(sub, FeXrRegImm16)->val = binop->lhs;
            fe_extra_T(sub, FeXrRegImm16)->num = as_u16(binop->rhs);
            return fe_new_chain(sub);
        } else {
            FeInst* sub = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
            sub->kind = FE_XR_SUB; sub->ty = FE_TY_I32;
            fe_extra_T(sub, FeXrRegReg)->lhs = binop->lhs;
            fe_extra_T(sub, FeXrRegReg)->rhs = binop->rhs;
            return fe_new_chain(sub);
        }
    case FE_IMUL: {
        FeInst* mul = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
        mul->kind = FE_XR_MUL;
        mul->ty = inst->ty;
        fe_extra_T(mul, FeXrRegReg)->lhs = binop->lhs;
        fe_extra_T(mul, FeXrRegReg)->rhs = binop->rhs;
        return fe_new_chain(mul);
    }
    case FE_RETURN: {
        FeInstReturn* inst_ret = extra;
        FeInst* ret = fe_ipool_alloc(f->ipool, 0);
        ret->kind = FE_XR_RET;
        ret->ty = FE_TY_VOID;
        FeInstChain chain = fe_new_chain(ret);
        for_n(i, 0, inst_ret->len) {
            FeInst* mov = fe_ipool_alloc(f->ipool, sizeof(FeInstUnop));
            mov->kind = FE_MOV_VOLATILE;
            mov->ty = FE_TY_VOID;
            mov->flags = 0;
            fe_extra_T(mov, FeInstUnop)->un = fe_return_arg(inst, i);
            // fe_insert_before(inst, mov);
            chain = fe_chain_append_begin(chain, mov);
        }
        return chain;
    }
    default:
        return fe_new_chain(inst);
    }
}