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
            fe_extra_T(addi, FeXrRegImm16)->reg = zero;
            fe_extra_T(addi, FeXrRegImm16)->num = as_u16(inst);
            FeInstChain chain = fe_new_chain(addi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } if (can_subi_const_u16(inst)) {
            // emit as a SUBI.
            FeInst* subi = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            subi->kind = FE_XR_SUBI; subi->ty = FE_TY_I32;
            FeInst* zero = fake_zero(f);
            fe_extra_T(subi, FeXrRegImm16)->reg = zero;
            fe_extra_T(subi, FeXrRegImm16)->num = as_subi_const_u16(inst);
            FeInstChain chain = fe_new_chain(subi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } else {
            // lui + addi pair
            FeInst* zero = fake_zero(f);

            FeInst* lui = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            lui->kind = FE_XR_LUI; lui->ty = FE_TY_I32;

            fe_extra_T(lui, FeXrRegImm16)->reg = zero;
            fe_extra_T(lui, FeXrRegImm16)->num = as_hi_u16(inst);


            FeInst* addi = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            addi->kind = FE_XR_ADDI; addi->ty = FE_TY_I32;
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
            FeInst* add = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            add->kind = FE_XR_ADDI; add->ty = FE_TY_I32;
            fe_extra_T(add, FeXrRegImm16)->reg = binop->lhs;
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
            fe_extra_T(sub, FeXrRegImm16)->reg = binop->lhs;
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
        mul->kind = FE_XR_MUL; mul->ty = FE_TY_I32;
        fe_extra_T(mul, FeXrRegReg)->lhs = binop->lhs;
        fe_extra_T(mul, FeXrRegReg)->rhs = binop->rhs;
        return fe_new_chain(mul);
    }
    case FE_BRANCH: {
        if (fe_extra_T(inst, FeInstBranch)->cond->kind == FE_EQ) {
            FeInstBinop* cmp_eq = fe_extra(fe_extra_T(inst, FeInstBranch)->cond);

            FeInst* beq = fe_ipool_alloc(f->ipool, sizeof(FeXrRegBranch));
            beq->kind = FE_XR_BEQ; beq->ty = FE_TY_VOID;
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
    } break;
    case FE_RETURN: {
        FeInstReturn* inst_ret = extra;
        FeInst* ret = fe_ipool_alloc(f->ipool, 0);
        ret->kind = FE_XR_RET;
        ret->ty = FE_TY_VOID;
        FeInstChain chain = fe_new_chain(ret);
        for_n(i, 0, inst_ret->len) {
            FeInst* mov = fe_ipool_alloc(f->ipool, sizeof(FeInstUnop));
            mov->kind = FE_MOV_VOLATILE;
            mov->ty = FE_TY_I32;
            mov->flags = 0;
            fe_extra_T(mov, FeInstUnop)->un = fe_return_arg(inst, i);
            chain = fe_chain_append_begin(chain, mov);
        }
        return chain;
    }
    }
    return fe_new_chain(inst);
}