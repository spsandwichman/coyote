#include "iron/iron.h"

// static bool use_is(FeInst* inst, usize count) {
//     return inst->use_count == count;
// }

static bool can_const_u16(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val <= UINT16_MAX;
}

static u16 as_u16(FeInst* inst) {
    return (u16)fe_extra_T(inst, FeInstConst)->val;
}

// static void remove_use(FeFunction* f, FeInst* inst) {
//     if (inst->use_count <= 1) {
//         fe_inst_free(f, fe_inst_remove(inst));
//     } else {
//         inst->use_count--;
//     }
// }

void fe_replace(FeInst* from, FeInst* to) {
    from->next->prev = to;
    from->prev->next = to;
    to->next = from->next;
    to->prev = from->prev;
}

static FeInst* isel(FeFunction* f, FeInst* inst) {
    void* extra = fe_extra(inst);
    FeInstBinop* binop = extra;
    FeInstReturn* ret = extra;

    FeInst* sel = NULL;

    switch (inst->kind) {
    case FE_IADD:
        if (can_const_u16(binop->rhs)) {
            sel = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            sel->kind = FE_XR_ADDI;
            sel->ty = inst->ty;
            fe_extra_T(sel, FeXrRegImm16)->val = binop->lhs;
            fe_extra_T(sel, FeXrRegImm16)->num = as_u16(binop->rhs);
        } else {
            sel = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
            sel->kind = FE_XR_ADD;
            sel->ty = inst->ty;
            fe_extra_T(sel, FeXrRegReg)->lhs = binop->lhs;
            fe_extra_T(sel, FeXrRegReg)->rhs = binop->rhs;
        }
        break;
    case FE_ISUB:
        if (can_const_u16(binop->rhs)) {
            sel = fe_ipool_alloc(f->ipool, sizeof(FeXrRegImm16));
            sel->kind = FE_XR_SUBI;
            sel->ty = inst->ty;
            fe_extra_T(sel, FeXrRegImm16)->val = binop->lhs;
            fe_extra_T(sel, FeXrRegImm16)->num = as_u16(binop->rhs);
        } else {
            sel = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
            sel->kind = FE_XR_SUB;
            sel->ty = inst->ty;
            fe_extra_T(sel, FeXrRegReg)->lhs = binop->lhs;
            fe_extra_T(sel, FeXrRegReg)->rhs = binop->rhs;
        }
        break;
    case FE_IMUL:
        sel = fe_ipool_alloc(f->ipool, sizeof(FeXrRegReg));
        sel->kind = FE_XR_MUL;
        sel->ty = inst->ty;
        fe_extra_T(sel, FeXrRegReg)->lhs = binop->lhs;
        fe_extra_T(sel, FeXrRegReg)->rhs = binop->rhs;
        break;
    case FE_RETURN:
        sel = fe_ipool_alloc(f->ipool, 0);
        sel->kind = FE_XR_RET;
        sel->ty = FE_TY_VOID;
        for_n(i, 0, ret->len) {
            FeInst* mov = fe_ipool_alloc(f->ipool, sizeof(FeInstUnop));
            mov->kind = FE_MOV_VOLATILE;
            mov->ty = FE_TY_VOID;
            mov->flags = 0;
            fe_extra_T(mov, FeInstUnop)->un = fe_return_arg(inst, i);
            fe_insert_before(inst, mov);
        }
        // fe_replace(inst, sel);
        break;
    default:
        return inst;
    }
    sel->flags = 0;
    return sel;
}

typedef struct FeInstPair {
    FeInst* to;
    FeInst* from;
} FeInstPair;

void fe_xr_isel(FeFunction* f) {
    fe_inst_update_uses(f);

    // assign each instruction an index starting from 1.
    const usize START = 1;
    usize inst_count = START;
    for_blocks(block, f) {
        for_inst(inst, block) {
            inst->flags = inst_count++;
        }
    }

    FeInstPair* isel_map = fe_malloc(sizeof(*isel_map) * inst_count);
    memset(isel_map, 0, sizeof(*isel_map) * inst_count);

    for_blocks(block, f) {
        for_inst_reverse(inst, block) {
            FeInst* sel = isel(f, inst);
            isel_map[inst->flags].from = inst;
            isel_map[inst->flags].to = sel;
        }
    }

    // replace instructions with selected instructions
    for_n(i, START, inst_count) {
        fe_replace(isel_map[i].from, isel_map[i].to);
    }

    // replace inputs to all selected instructions
    for_blocks(block, f) {
        for_inst(inst, block) {
            usize inputs_len = 0;
            FeInst** inputs = fe_inst_list_inputs(inst, &inputs_len);
            for_n(i, 0, inputs_len) {
                if (inputs[i]->kind == FE_PARAM) continue;
                if (inputs[i]->flags == 0) continue;

                inputs[i] = isel_map[inputs[i]->flags].to;
            }
        }
    }

    for_n(i, START, inst_count) {
        if (isel_map[i].from != isel_map[i].to) {
            fe_inst_free(f, isel_map[i].from);
        }
    }

    fe_free(isel_map);

    printf("isel complete\n");
}