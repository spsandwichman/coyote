#include "iron/iron.h"
#include "xr.h"

static bool is_const_zero(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val == 0;
}

static bool can_const_u5(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val <= 0b11111;
}

static u8 as_const_u5(FeInst* inst) {
    return fe_extra_T(inst, FeInstConst)->val;
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

static FeInst* mach_reg(FeFunc* f, FeBlock* block, u16 real_reg) {
    FeInst* zero = fe_ipool_alloc(f->ipool, 0);
    zero->kind = FE__MACH_REG;
    zero->ty = FE_TY_I32;
    FeVReg zero_reg = fe_vreg_new(f->vregs, zero, block, XR_REGCLASS_REG);
    fe_vreg(f->vregs, zero_reg)->real = real_reg;
    return zero;
}

static FeInst* create_mach(FeFunc* f, FeInstKind kind, FeTy ty, usize size) {
    FeInst* i = fe_ipool_alloc(f->ipool, size);
    memset(fe_extra(i), 0, size);
    i->kind = kind; 
    i->ty = ty;
    i->flags = FE_ISEL_GENERATED;
    // i->vr_out = (ty == FE_TY_TUPLE || ty == FE_TY_VOID) ? FE_VREG_NONE : fe_vreg_new(f->vregs, i, XR_REGCLASS_REG);
    i->vr_out = FE_VREG_NONE;
    return i;
}

static FeInst* mach_mov(FeFunc* f, FeInst* source) {
    FeInst* i = fe_inst_unop(f, FE_TY_I32, FE__MACH_MOV, source);
    i->flags = FE_ISEL_GENERATED;
    // i->vr_out = fe_vreg_new(f->vregs, i, XR_REGCLASS_REG);
    i->vr_out = FE_VREG_NONE;
    return i;
}

static void preassign(FeFunc* f, FeInst* inst, FeBlock* block, u16 real_reg) {
    if (inst->vr_out == FE_VREG_NONE) {
        inst->vr_out = fe_vreg_new(f->vregs, inst, block, XR_REGCLASS_REG);
    }
    fe_vreg(f->vregs, inst->vr_out)->real = real_reg;
}

// static void set_hint(FeFunc* f, FeInst* inst, FeBlock* block, u16 real_reg) {
//     if (inst->vr_out == FE_VREG_NONE) {
//         inst->vr_out = fe_vreg_new(f->vregs, inst, block, XR_REGCLASS_REG);
//     }
//     fe_vreg(f->vregs, inst->vr_out)->hint = real_reg;
// }

FeInstChain xr_isel(FeFunc* f, FeBlock* block, FeInst* inst) {
    void* extra = fe_extra(inst);
    FeInstBinop* binop = extra;
    const FeTarget* target = f->mod->target;

    switch (inst->kind) {
    case FE_PARAM: {
        // params become a mach_reg and mach_mov

        u16 real_param_reg = XR_REG_ZERO;
        switch (fe_extra_T(inst, FeInstParam)->index) {
        case 0: real_param_reg = XR_REG_A0; break;
        case 1: real_param_reg = XR_REG_A1; break;
        case 2: real_param_reg = XR_REG_A2; break;
        case 3: real_param_reg = XR_REG_A3; break;
        default:
            FE_CRASH("pass on stack later LOL");
        }

        FeInst* param_reg = mach_reg(f, block, real_param_reg);
        FeInst* mov = mach_mov(f, param_reg);
        // set_hint(f, mov, block, real_param_reg);
        FeInstChain chain = fe_chain_new(param_reg);
        chain = fe_chain_append_end(chain, mov);
        return chain;
    }
    case FE_CONST:
        if (can_const_u16(inst)) {
            // emit as addi zero.
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);
            FeInst* addi = create_mach(f, XR_ADDI, FE_TY_I32, sizeof(XrRegImm16));
            fe_extra_T(addi, XrRegImm16)->reg = zero;
            fe_extra_T(addi, XrRegImm16)->imm16 = as_u16(inst);
            FeInstChain chain = fe_chain_new(addi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } if (can_subi_const_u16(inst)) {
            // emit as subi zero
            FeInst* subi = create_mach(f, XR_SUBI, FE_TY_I32, sizeof(XrRegImm16));
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);
            fe_extra_T(subi, XrRegImm16)->reg = zero;
            fe_extra_T(subi, XrRegImm16)->imm16 = as_subi_const_u16(inst);
            FeInstChain chain = fe_chain_new(subi);
            chain = fe_chain_append_begin(chain, zero);
            return chain;
        } else {
            // lui + addi pair
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);

            FeInst* lui = create_mach(f, XR_LUI, FE_TY_I32, sizeof(XrRegImm16));

            fe_extra_T(lui, XrRegImm16)->reg = zero;
            fe_extra_T(lui, XrRegImm16)->imm16 = as_hi_u16(inst);

            FeInst* addi = create_mach(f, XR_ADDI, FE_TY_I32, sizeof(XrRegImm16));
            fe_extra_T(addi, XrRegImm16)->reg = lui;
            fe_extra_T(addi, XrRegImm16)->imm16 = as_u16(inst);

            FeInstChain chain = fe_chain_new(zero);
            chain = fe_chain_append_end(chain, lui);
            chain = fe_chain_append_end(chain, addi);
            return chain;
        }
        return fe_chain_new(inst);
    case FE_IADD:
        if (can_const_u16(binop->rhs)) {
            FeInst* addi = create_mach(f, XR_ADDI, FE_TY_I32, sizeof(XrRegImm16));
            fe_extra_T(addi, XrRegImm16)->reg = binop->lhs;
            fe_extra_T(addi, XrRegImm16)->imm16 = as_u16(binop->rhs);
            return fe_chain_new(addi);
        } else {
            FeInst* add = create_mach(f, XR_ADD, FE_TY_I32, sizeof(XrRegReg));
            fe_extra_T(add, XrRegReg)->r1 = binop->lhs;
            fe_extra_T(add, XrRegReg)->r2 = binop->rhs;
            return fe_chain_new(add);
        }
        break;
    case FE_ISUB:
        if (can_const_u16(binop->rhs)) {
            FeInst* sub = create_mach(f, XR_SUBI, FE_TY_I32, sizeof(XrRegImm16));
            fe_extra_T(sub, XrRegImm16)->reg = binop->lhs;
            fe_extra_T(sub, XrRegImm16)->imm16 = as_u16(binop->rhs);
            return fe_chain_new(sub);
        } else {
            FeInst* sub = create_mach(f, XR_SUB, FE_TY_I32, sizeof(XrRegReg));
            fe_extra_T(sub, XrRegReg)->r1 = binop->lhs;
            fe_extra_T(sub, XrRegReg)->r2 = binop->rhs;
            return fe_chain_new(sub);
        }
    case FE_IMUL: {
        FeInst* mul = create_mach(f, XR_MUL, FE_TY_I32, sizeof(XrRegReg));
        fe_extra_T(mul, XrRegReg)->r1 = binop->lhs;
        fe_extra_T(mul, XrRegReg)->r2 = binop->rhs;
        return fe_chain_new(mul);
    }
    case FE_IDIV: {
        FeInst* div = create_mach(f, XR_DIV_SIGNED, FE_TY_I32, sizeof(XrRegReg));
        fe_extra_T(div, XrRegReg)->r1 = binop->lhs;
        fe_extra_T(div, XrRegReg)->r2 = binop->rhs;
        return fe_chain_new(div);
    }
    case FE_UDIV: {
        FeInst* div = create_mach(f, XR_DIV, FE_TY_I32, sizeof(XrRegReg));
        fe_extra_T(div, XrRegReg)->r1 = binop->lhs;
        fe_extra_T(div, XrRegReg)->r2 = binop->rhs;
        return fe_chain_new(div);
    }
    case FE_SHL:
        if (can_const_u5(binop->rhs)) {
            // xr.add zero, %1 lsh <const>
            FeInst* zero = mach_reg(f, block, XR_REG_ZERO);
            FeInst* add = create_mach(f, XR_ADD, FE_TY_I32, sizeof(XrRegReg));
            fe_extra_T(add, XrRegReg)->r1 = zero;
            fe_extra_T(add, XrRegReg)->r2 = binop->lhs;
            fe_extra_T(add, XrRegReg)->shift_kind = XR_SHIFT_LSH;
            fe_extra_T(add, XrRegReg)->imm5 = as_const_u5(binop->rhs);

            FeInstChain chain = fe_chain_new(zero);
            chain = fe_chain_append_end(chain, add);
            return chain;
        } else {
            FeInst* shift = create_mach(f, XR_SHIFT, FE_TY_I32, sizeof(XrRegReg));
            fe_extra_T(shift, XrRegReg)->r1 = binop->lhs;
            fe_extra_T(shift, XrRegReg)->r2 = binop->rhs;
            fe_extra_T(shift, XrRegReg)->shift_kind = XR_SHIFT_LSH;
            return fe_chain_new(shift);
        }
    case FE_IEQ: {
        //  %2 = eq %0, %1
        // ->
        //  %2 = xr.sub  %0, %1
        //  %3 = xr.slti %2, 1
        FeInst* sub = create_mach(f, XR_SUB, FE_TY_I32, sizeof(XrRegReg));
        fe_extra_T(sub, XrRegReg)->r1 = binop->lhs;
        fe_extra_T(sub, XrRegReg)->r2 = binop->rhs;
        FeInst* slti = create_mach(f, XR_SLTI, FE_TY_I32, sizeof(XrRegImm16));
        fe_extra_T(slti, XrRegImm16)->reg = sub;
        fe_extra_T(slti, XrRegImm16)->imm16 = 1;

        FeInstChain chain = fe_chain_new(sub);
        chain = fe_chain_append_end(chain, slti);
        return chain;
    }
    case FE_BRANCH: {
        FeInst* condition = fe_extra_T(inst, FeInstBranch)->cond;
        FeBlock* if_true = fe_extra_T(inst, FeInstBranch)->if_true;
        FeBlock* if_false = fe_extra_T(inst, FeInstBranch)->if_false;
        ;
        if (condition->kind == FE_IEQ) {
            FeInstBinop* cmp_eq = fe_extra(condition);

            FeInst* beq = create_mach(f, XR_BEQ, FE_TY_VOID, sizeof(XrRegBranch));
            FeInstChain chain = fe_chain_new(beq);

            FeInst* result;
            if (is_const_zero(cmp_eq->rhs)) {
                //  xr.beq %0, 1:, 2:
                result = cmp_eq->lhs;
            } else if (can_const_u16(cmp_eq->rhs)) {
                //  %2: i32 = xr.subi %0, CONST16
                //  xr.beq %2, 1:, 2:
                result = fe_ipool_alloc(f->ipool, sizeof(XrRegImm16));
                result->kind = XR_SUBI; result->ty = FE_TY_I32;
                fe_extra_T(result, XrRegImm16)->reg = cmp_eq->lhs;
                fe_extra_T(result, XrRegImm16)->imm16 = as_u16(cmp_eq->rhs);
                chain = fe_chain_append_begin(chain, result);
            } else {
                //  %2: i32 = xr.sub %0, %1
                //  xr.beq %2, 1:, 2:
                result = fe_ipool_alloc(f->ipool, sizeof(XrRegReg));
                result->kind = XR_SUB; result->ty = FE_TY_I32;
                fe_extra_T(result, XrRegReg)->r1 = cmp_eq->lhs;
                fe_extra_T(result, XrRegReg)->r2 = cmp_eq->rhs;
                chain = fe_chain_append_begin(chain, result);
            }

            fe_extra_T(beq, XrRegBranch)->reg = result;
            fe_extra_T(beq, XrRegBranch)->dest = if_true;
            fe_extra_T(beq, XrRegBranch)->_else = if_false;

            return chain;
        } else {
            // just branch-not-equal (not-zero)
            FeInst* bne = create_mach(f, XR_BNE, FE_TY_VOID, sizeof(XrRegBranch));
            fe_extra_T(bne, XrRegBranch)->reg = condition;
            fe_extra_T(bne, XrRegBranch)->dest = if_true;
            fe_extra_T(bne, XrRegBranch)->_else = if_false;
            return fe_chain_new(bne);
        }
        // return fe_chain_new(inst);
    }
    case FE_JUMP: {
        FeInst* j = create_mach(f, XR_J, FE_TY_VOID, sizeof(XrJump));
        fe_extra_T(j, XrJump)->dest = fe_extra_T(inst, FeInstJump)->to;
        return fe_chain_new(j);
    }
    case FE_RETURN: {
        FeInstReturn* inst_ret = extra;
        FeInst* ret = fe_ipool_alloc(f->ipool, 0);
        ret->kind = XR_RET;
        ret->ty = FE_TY_VOID;
        FeInstChain chain = fe_chain_new(ret);
        for_n(i, 0, inst_ret->len) {
            FeInst* arg = fe_return_arg(inst, i);
            FeInst* mov = mach_mov(f, arg);
            
            u16 real_return_reg = XR_REG_ZERO;
            switch (i) {
            case 0: real_return_reg = XR_REG_A3; break;
            case 1: real_return_reg = XR_REG_A2; break;
            case 2: real_return_reg = XR_REG_A1; break;
            case 3: real_return_reg = XR_REG_A0; break;
            default:
                FE_CRASH("pass on stack later LOL");
            }

            preassign(f, mov, block, real_return_reg);
            chain = fe_chain_append_begin(chain, mov);
        }
        return chain;
    }
    case FE_MOV:
    case FE__MACH_MOV:
    case FE_PHI:
    case FE_UPSILON:
        return fe_chain_new(inst);
    }
    FE_CRASH("xr_isel: unable to select inst kind %s (%u)", fe_inst_name(target, inst->kind), inst->kind);
    return fe_chain_new(inst);
}

static u16 reg_num(FeFunc* f, FeInst* inst) {
    FeVirtualReg* vr = fe_vreg(f->vregs, inst->vr_out);
    return vr->real;
}

static u16 branch_inv_kind(u16 kind) {
    switch (kind) {
    case XR_BEQ: return XR_BNE;
    case XR_BNE: return XR_BEQ;
    case XR_BLT: return XR_BGE;
    case XR_BGE: return XR_BLT;
    case XR_BGT: return XR_BLE;
    case XR_BLE: return XR_BGT;
    default:
        return 0;
    }
}

// final, post-regalloc touchups begin.
// after this, the function is not necessarily valid SSA
// nor valid IR at all.
// here be dragons!!!!!
void xr_final_touchups(FeFunc* f) {

    bool should_push_lr = false;

    // if this function contains a call, push the link register to the top of the stack frame.
    FeStackItem* lr_slot = nullptr;
    if (should_push_lr) {
        lr_slot = fe_stack_item_new(FE_TY_I32, nullptr);
        fe_stack_append_top(f, lr_slot);
    }

    u32 stack_size = fe_stack_calculate_size(f);

    // add stack spill at the beginning
    FeInst* lr = mach_reg(f, f->entry_block, XR_REG_LR);
    if (should_push_lr) {
        FeInst* spill_lr = fe_ipool_alloc(f->ipool, sizeof(FeInst__MachStackSpill));
        spill_lr->ty = FE_TY_VOID; spill_lr->kind = FE__MACH_STACK_SPILL;
        fe_extra_T(spill_lr, FeInst__MachStackSpill)->val = lr;
        fe_extra_T(spill_lr, FeInst__MachStackSpill)->item = lr_slot;
        fe_append_begin(f->entry_block, spill_lr);
    }


    // set up the stack
    FeInst* sp = mach_reg(f, f->entry_block, XR_REG_SP);
    if (stack_size != 0) {
        FeInst* stack_setup = create_mach(f, XR_SUBI, FE_TY_I32, sizeof(XrRegImm16));
        preassign(f, stack_setup, f->entry_block, XR_REG_SP);
        fe_extra_T(stack_setup, XrRegImm16)->reg = sp;
        fe_extra_T(stack_setup, XrRegImm16)->imm16 = stack_size;
        fe_append_begin(f->entry_block, stack_setup);

    }    

    // restore lr and sp at returns.
    for_blocks(block, f) {
        FeInst* ret = block->bookend->prev;
        if (ret->kind != XR_RET) continue;

        FeInst* reload_lr = nullptr;
        if (should_push_lr) {
            reload_lr = fe_ipool_alloc(f->ipool, sizeof(FeInst__MachStackReload));
            reload_lr->ty = FE_TY_I32; reload_lr->kind = FE__MACH_STACK_RELOAD;
            fe_extra_T(reload_lr, FeInst__MachStackReload)->item = lr_slot;
            preassign(f, reload_lr, block, XR_REG_LR);
            fe_insert_before(ret, reload_lr);
        }

        if (stack_size != 0) {
            FeInst* stack_destroy = create_mach(f, XR_ADDI, FE_TY_I32, sizeof(XrRegImm16));
            preassign(f, stack_destroy, f->entry_block, XR_REG_SP);
            fe_extra_T(stack_destroy, XrRegImm16)->reg = sp;
            fe_extra_T(stack_destroy, XrRegImm16)->imm16 = stack_size;
            fe_insert_before(ret, stack_destroy);
        }
    }

    // normalize plenty of different instructions
    for_blocks(block, f) {
        for_inst(inst, block) {
            FeInstUnop* unop = fe_extra(inst);

            switch (inst->kind) {
            case FE_UPSILON:
                inst->kind = FE__MACH_MOV;
                [[fallthrough]];
            case FE__MACH_MOV:
                if (reg_num(f, inst) == reg_num(f, unop->un)) {
                    fe_inst_remove_pos(inst);
                } else {
                    // turn it into a regular XR mov
                    FeInst* addi = create_mach(f, XR_MOV, FE_TY_I32, sizeof(XrRegImm16));
                    fe_extra_T(addi, XrRegImm16)->reg = unop->un;
                    addi->vr_out = inst->vr_out;
                    fe_inst_replace_pos(inst, addi);
                }
                break;
            case FE__MACH_REG:
            case FE_PHI:
                fe_inst_remove_pos(inst);
                break;
            case FE__MACH_STACK_SPILL:
                // turn it into a store to sp
                ;
                FeInst* spill = create_mach(f, XR_STORE32_IMM, inst->ty, sizeof(XrRegRegImm16));
                // reload->vr_out = inst->vr_out;
                fe_extra_T(spill, XrRegRegImm16)->r1 = sp;
                fe_extra_T(spill, XrRegRegImm16)->imm16 = fe_extra_T(inst, FeInst__MachStackSpill)->item->_offset;
                fe_extra_T(spill, XrRegRegImm16)->r2 = fe_extra_T(inst, FeInst__MachStackSpill)->val;
                fe_inst_replace_pos(inst, spill);
                break;
            case FE__MACH_STACK_RELOAD:
                // turn it into a load from sp
                ;
                FeInst* reload = create_mach(f, XR_LOAD32_IMM, inst->ty, sizeof(XrRegImm16));
                reload->vr_out = inst->vr_out;
                fe_extra_T(reload, XrRegImm16)->reg = sp;
                fe_extra_T(reload, XrRegImm16)->imm16 = fe_extra_T(inst, FeInst__MachStackReload)->item->_offset;
                fe_inst_replace_pos(inst, reload);
                break;
            default: 
                continue;
            }
        }
    }

    // number blocks rq
    u32 block_counter = 0;
    for_blocks(block, f) {
        block->flags = block_counter++;
    }

    // optimize/remove branches
    for_blocks(block, f) {
        FeInst* inst = block->bookend->prev;
        
        // if its a jump to the very next block, remove it
        if (inst->kind == XR_J && fe_extra_T(inst, XrJump)->dest->flags == block->flags + 1) {
            fe_inst_remove_pos(inst);
            continue;
        }

        if (branch_inv_kind(inst->kind) == 0)
            continue;
        
        XrRegBranch* br = fe_extra(inst);
        
        // if the dest block is right after a conditional jump
        // reverse its condition and its destination
        if (br->dest->flags == block->flags + 1) {
            inst->kind = branch_inv_kind(inst->kind);
            FeBlock* _else = br->_else;
            br->_else = br->dest;
            br->dest = _else;
        }
    }
}
