#include "iron/iron.h"

FeModule* fe_new_module(FeArch arch, FeSystem system) {
    FeModule* mod = fe_malloc(sizeof(FeModule));
    memset(mod, 0, sizeof(FeModule));
    mod->target = fe_make_target(arch, system);
    return mod;
}

// if len == 0, calculate with strlen
FeSymbol* fe_new_symbol(FeModule* m, const char* name, u16 len, FeSymbolBinding bind) {
    if (len == 0) {
        len = strlen(name);
    }
    // put this in an arena of some sort later
    FeSymbol* sym = fe_malloc(sizeof(FeSymbol));
    sym->kind = FE_SYMKIND_EXTERN;
    sym->bind = bind;
    sym->name = name;
    sym->name_len = len;

    return sym;
}

FeFuncSignature* fe_new_funcsig(FeCallConv cconv, u16 param_len, u16 return_len) {
    FeFuncSignature* sig = fe_malloc(
        sizeof(FeFuncSignature) + (param_len + return_len) * sizeof(sig->params[0])
    );
    sig->cconv = cconv,
    sig->param_len = param_len;
    sig->return_len = return_len;
    return sig;
};

FeFuncParam* fe_funcsig_param(FeFuncSignature* sig, usize index) {
    if (index >= sig->param_len) {
        fe_runtime_crash("index > param_len");
    }
    return &sig->params[index];
}

FeFuncParam* fe_funcsig_return(FeFuncSignature* sig, usize index) {
    if (index >= sig->param_len) {
        fe_runtime_crash("index > param_len");
    }
    return &sig->params[sig->param_len + index];
}

FeBlock* fe_new_block(FeFunction* f) {
    FeBlock* block = fe_malloc(sizeof(FeBlock));
    memset(block, 0, sizeof(*block));
    block->list_next = NULL;
    block->list_prev = NULL;
    
    // adds initial bookend instruction to block
    FeInst* bookend = fe_ipool_alloc(f->ipool, sizeof(FeInstBookend));
    bookend->kind = FE_BOOKEND;
    bookend->ty = FE_TY_VOID;
    bookend->next = bookend;
    bookend->prev = bookend;
    fe_extra_T(bookend, FeInstBookend)->block = block;
    block->bookend = bookend;

    // append to block list
    if (f->last_block) {
        FeBlock* last = f->last_block;
        block->list_prev = last;
        last->list_next = block;
        f->last_block = block;
    }
    return block;
}

FeFunction* fe_new_function(FeModule* mod, FeSymbol* sym, FeFuncSignature* sig, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeFunction* f = fe_malloc(sizeof(FeFunction));
    memset(f, 0, sizeof(FeFunction));
    f->sig = sig;
    f->mod = mod;
    f->ipool = ipool;
    f->vregs = vregs;
    f->sym = sym;
    sym->func = f;
    sym->kind = FE_SYMKIND_FUNC;

    // append to function list
    f->list_prev = mod->funcs.first;
    if (mod->funcs.first) mod->funcs.first->list_next = f;
    else mod->funcs.last = f;
    mod->funcs.first = f;

    // add initial basic block
    f->entry_block = f->last_block = fe_new_block(f);

    f->params = fe_malloc(sizeof(f->params[0]) * sig->param_len);

    // adds parameter instructions
    for_n(i, 0, sig->param_len) {
        FeInst* param = fe_ipool_alloc(ipool, sizeof(FeInstParam));
        param->prev = NULL;
        param->next = NULL;
        if (i != 0) {
            param->prev = f->params[i - 1];
            param->prev->next = param;
        }
        param->kind = FE_PARAM;
        param->ty = sig->params[i].ty;
        fe_extra_T(param, FeInstParam)->index = i;
        fe_append_end(f->entry_block, param);
        f->params[i] = param;
    }

    return f;
}

FeInst* fe_func_param(FeFunction* f, usize index) {
    return f->params[index];
}
void fe_inst_update_uses(FeFunction* f) {
    FeTarget* t = f->mod->target;

    for_blocks(block, f) {
        for_inst(inst, block) {
            inst->use_count = 0;
        }
    }
    for_blocks(block, f) {
        for_inst(inst, block) {
            usize len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &len);
            for_n (i, 0, len) {
                inputs[i]->use_count++;
            }
        }
    }
}

// jank as FUCK and very likely to break; too bad!
FeInst** fe_inst_list_inputs(FeTarget* t, FeInst* inst, usize* len_out) {
    if (inst->kind > _FE_BASE_INST_END) {
        return t->list_inputs(inst, len_out);
    }

    switch (inst->kind) {
    case FE_PROJ:
    case FE_PROJ_VOLATILE:
        *len_out = 1;
        return &fe_extra_T(inst, FeInstProj)->val;
    case FE_IADD ... FE_FREM:
        *len_out = 2;
        return &fe_extra_T(inst, FeInstBinop)->lhs;
    case FE_MOV ... FE_F2I:
        *len_out = 1;
        return &fe_extra_T(inst, FeInstUnop)->un;
    case FE_LOAD ... FE_LOAD_VOLATILE:
        *len_out = 1;
        return &fe_extra_T(inst, FeInstLoad)->ptr;
    case FE_STORE ... FE_STORE_VOLATILE:
        *len_out = 2;
        return &fe_extra_T(inst, FeInstStore)->ptr;
    case FE_BRANCH:
        *len_out = 1;
        return &fe_extra_T(inst, FeInstBranch)->cond;
    case FE_CALL_DIRECT:  
    case FE_CALL_INDIRECT:
        FeInstCallDirect* call = fe_extra(inst);
        *len_out = call->len;
        if (call->cap == 0) {
            return &call->single_arg;
        } else {
            return call->multi_arg;
        }
    case FE_RETURN:
        FeInstReturn* ret = fe_extra(inst);
        *len_out = ret->len;
        if (ret->cap == 0) {
            return &ret->single;
        } else {
            return ret->multi;
        }
        break;
    case FE_BOOKEND:
    case FE_PARAM:
    case FE_CONST:
    case FE_CASCADE_UNIQUE:
    case FE_CASCADE_VOLATILE:
    case FE_JUMP:
    case FE_MACH_REG:
        *len_out = 0;
        return NULL;
    default:
        fe_runtime_crash("list_inputs: unknown kind %d", inst->kind);
        break;
    }
}

FeBlock** fe_inst_term_list_targets(FeTarget* t, FeInst* term, usize* len_out) {
    if (!fe_inst_has_trait(term->kind, FE_TRAIT_TERMINATOR)) {
        fe_runtime_crash("list_targets: inst %d is not a terminator", term->kind);
    }
    if (term->kind > _FE_BASE_INST_END) {
        return t->list_targets(term, len_out);
    }

    switch (term->kind) {
    case FE_JUMP:
        *len_out = 1;
        return &fe_extra_T(term, FeInstJump)->to;
    case FE_BRANCH:
        *len_out = 2;
        return &fe_extra_T(term, FeInstBranch)->if_true;
    case FE_RETURN:
    default:
        *len_out = 0;
        return NULL;
    }
}

void fe_inst_free(FeFunction* f, FeInst* inst) {
    fe_ipool_free(f->ipool, inst);
}

FeInst* fe_inst_remove(FeInst* inst) {
    inst->next->prev = inst->prev;
    inst->prev->next = inst->next;
    inst->next = NULL;
    inst->prev = NULL;
    return inst;
}

FeInst* fe_insert_before(FeInst* point, FeInst* i) {
    FeInst* p_prev = point->prev;
    p_prev->next = i;
    point->prev = i;
    i->next = point;
    i->prev = p_prev;
    return i;
}

FeInst* fe_insert_after(FeInst* point, FeInst* i) {
    FeInst* p_next = point->next;
    p_next->prev = i;
    point->next = i;
    i->prev = point;
    i->next = p_next;
    return i;
}

void fe_inst_replace_pos(FeInst* from, FeInst* to) {
    from->next->prev = to;
    from->prev->next = to;
    to->next = from->next;
    to->prev = from->prev;
}

FeInstChain fe_new_chain(FeInst* initial) {
    FeInstChain chain = {0};
    chain.begin = initial;
    chain.end = initial;
    return chain;
}

FeInstChain fe_chain_append_end(FeInstChain chain, FeInst* i) {
    chain.end->next = i;
    i->prev = chain.end;
    chain.end = i;
    return chain;
}

FeInstChain fe_chain_append_begin(FeInstChain chain, FeInst* i) {
    chain.begin->prev = i;
    i->next = chain.begin;
    chain.begin = i;
    return chain;
}

void fe_insert_chain_before(FeInst* point, FeInstChain chain) {
    FeInst* p_prev = point->prev;
    p_prev->next = chain.begin;
    point->prev = chain.end;
    chain.end->next = point;
    chain.begin->prev = p_prev;
}

void fe_insert_chain_after(FeInst* point, FeInstChain chain) {
    FeInst* p_next = point->next;
    p_next->prev = chain.end;
    point->next = chain.begin;
    chain.begin->prev = point;
    chain.end->next = p_next;
}

void fe_chain_replace_pos(FeInst* from, FeInstChain to) {
    from->next->prev = to.end;
    from->prev->next = to.begin;
    to.end->next = from->next;
    to.begin->prev = from->prev;
}

FeInst* fe_inst_const(FeFunction* f, FeTy ty, u64 val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstConst));
    inst->kind = FE_CONST;
    inst->ty = ty;
    fe_extra_T(inst, FeInstConst)->val = val;
    return inst;
}

FeInst* fe_inst_unop(FeFunction* f, FeTy ty, FeInstKind kind, FeInst* val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstUnop));
    inst->kind = kind;
    inst->ty = ty;
    fe_extra_T(inst, FeInstUnop)->un = val;
    return inst;
}

FeInst* fe_inst_binop(FeFunction* f, FeTy ty, FeInstKind kind, FeInst* lhs, FeInst* rhs) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstBinop));
    inst->kind = kind;
    inst->ty = ty;

    if (lhs->kind == FE_CONST && fe_inst_has_trait(kind, FE_TRAIT_COMMUTATIVE)) {
        FeInst* temp = lhs;
        lhs = rhs;
        rhs = temp;
    }
    if (fe_inst_has_trait(kind, FE_TRAIT_BOOL_OUT_TY)) {
        inst->ty = FE_TY_BOOL;
    }

    fe_extra_T(inst, FeInstBinop)->lhs = lhs;
    fe_extra_T(inst, FeInstBinop)->rhs = rhs;
    return inst;
}

FeInst* fe_inst_bare(FeFunction* f, FeTy ty, FeInstKind kind) {
    FeInst* inst = fe_ipool_alloc(f->ipool, 0);
    inst->kind = kind;
    inst->ty = ty;
    return inst;
}

FeInst* fe_inst_return(FeFunction* f) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstReturn));
    inst->kind = FE_RETURN;
    FeInstReturn* ret = fe_extra_T(inst, FeInstReturn);

    usize num_returns = f->sig->return_len;
    ret->len = num_returns;
    if (num_returns < 2) {
        ret->cap = 0;
    } else {
        ret->cap = num_returns;
        // allocate buffer
        ret->multi = fe_malloc(sizeof(FeInst*) * num_returns);
    }
    return inst;
}

FeInst* fe_return_arg(FeInst* ret, usize index) {
    FeInstReturn* r = fe_extra_T(ret, FeInstReturn);
    if (index >= r->len) {
        fe_runtime_crash("index >= ret->len");
    }
    if (r->len <= 1) {
        return r->single;
    } else {
        return r->multi[index];
    }
}

void fe_set_return_arg(FeInst* ret, usize index, FeInst* arg) {
    FeInstReturn* r = fe_extra_T(ret, FeInstReturn);
    if (index >= r->len) {
        fe_runtime_crash("index >= ret->len");
    }
    
    // arg->use_count++; // since we dont do it earlier...

    if (r->len <= 1) {
        r->single = arg;
    } else {
        r->multi[index] = arg;
    }
}

FeInst* fe_inst_branch(FeFunction* f, FeInst* cond, FeBlock* if_true, FeBlock* if_false) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstBranch));
    inst->kind = FE_BRANCH;
    FeInstBranch* branch = fe_extra(inst);
    branch->cond = cond;
    branch->if_true = if_true;
    branch->if_false = if_false;
    return inst;
}

FeInst* fe_inst_jump(FeFunction* f, FeBlock* to) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstJump));
    inst->kind = FE_JUMP;
    FeInstJump* jump = fe_extra(inst);
    jump->to = to;
    return inst;
}

FeInst* fe_inst_call_direct(FeFunction* f, FeFunction* to_call) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstCallDirect));
    inst->kind = FE_CALL_DIRECT;
    FeInstCallDirect* call = fe_extra(inst);
    call->to_call = to_call;

    // set up parameters
    usize num_params = to_call->sig->param_len;
    call->len = num_params;
    if (num_params < 2) {
        call->cap = 0;
    } else {
        call->cap = num_params;
        // allocate buffer
        call->multi_arg = fe_malloc(sizeof(FeInst*) * num_params);
    }
    // set up return type
    if (to_call->sig->return_len == 0) {
        inst->ty = FE_TY_VOID;
    } else if (to_call->sig->return_len == 1) {
        inst->ty = fe_funcsig_return(to_call->sig, 0)->ty;
    } else {
        inst->ty = FE_TY_TUPLE; // use proj to get returns....
    }
    return inst;
}

FeInst* fe_inst_call_indirect(FeFunction* f, FeInst* to_call, FeFuncSignature* sig) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstCallIndirect));
    inst->kind = FE_CALL_INDIRECT;
    FeInstCallIndirect* call = fe_extra(inst);
    call->to_call = to_call;
    call->sig = sig;

    // set up parameters
    usize num_params = sig->param_len;
    call->len = num_params;
    if (num_params < 2) {
        call->cap = 0;
    } else {
        call->cap = num_params;
        // allocate buffer
        call->multi_arg = fe_malloc(sizeof(FeInst*) * num_params);
    }
    // set up return type
    if (sig->return_len == 0) {
        inst->ty = FE_TY_VOID;
    } else if (sig->return_len == 1) {
        inst->ty = fe_funcsig_return(sig, 0)->ty;
    } else {
        inst->ty = FE_TY_TUPLE; // use proj to get returns....
    }
    return inst;
}

FeInst* fe_call_arg(FeInst* call, usize index) {
    // direct/indirect calls have the same param list layout
    FeInstCallDirect* c = fe_extra(call);
    if (index >= c->len) {
        fe_runtime_crash("index >= ret->len");
    }
    if (c->len <= 1) {
        return c->single_arg;
    } else {
        return c->multi_arg[index];
    }
}

void fe_set_call_arg(FeInst* call, usize index, FeInst* arg) {
    // direct/indirect calls have the same param list layout, so its fine
    FeInstCallDirect* c = fe_extra(call);
    if (index >= c->len) {
        fe_runtime_crash("index >= ret->len");
    }
    
    // arg->use_count++; // since we dont do it earlier...

    if (c->len <= 1) {
        c->single_arg = arg;
    } else {
        c->multi_arg[index] = arg;
    }
}

FeTy fe_proj_ty(FeInst* tuple, usize index) {
    if (tuple->ty != FE_TY_TUPLE) {
        return FE_TY_VOID;
    }
    switch (tuple->kind) {
    case FE_CALL_DIRECT:
        FeInstCallDirect* dcall = fe_extra(tuple);
        if (index < dcall->to_call->sig->return_len) {
            return fe_funcsig_return(dcall->to_call->sig, index)->ty;
        }
        break;
    case FE_CALL_INDIRECT:
        FeInstCallIndirect* icall = fe_extra(tuple);
        if (index < icall->sig->return_len) {
            return fe_funcsig_return(icall->sig, index)->ty;
        }
        break;
    }
    return FE_TY_VOID;
}

#include "short_traits.h"

static FeTrait inst_traits[_FE_INST_END] = {
    [FE_PROJ] = 0,
    [FE_PROJ_VOLATILE] = VOL,
    [FE_PARAM] = VOL,
    [FE_CONST] = 0,

    [FE_IADD] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
    [FE_ISUB] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_IMUL] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
    [FE_UMUL] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
    [FE_IDIV] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_UDIV] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_IREM] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_UREM] = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_AND]  = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
    [FE_OR]   = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
    [FE_XOR]  = INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC,
    [FE_SHL]  = INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_USR]  = INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_ISR]  = INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_ILT]  = INT_IN | SAME_INS | BOOL_OUT,
    [FE_ULT]  = INT_IN | SAME_INS | BOOL_OUT,
    [FE_ILE]  = INT_IN | SAME_INS | BOOL_OUT,
    [FE_ULE]  = INT_IN | SAME_INS | BOOL_OUT,
    [FE_EQ]   = INT_IN | VEC_IN | SAME_INS | BOOL_OUT | COMMU,

    [FE_FADD] = FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | FAST_ASSOC | FAST_COMMU,
    [FE_FSUB] = FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_FMUL] = FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | FAST_ASSOC | FAST_COMMU,
    [FE_FDIV] = FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_FREM] = FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,

    [FE_MOV]          = SAME_IN_OUT,
    [FE_MACH_MOV] = VOL | SAME_IN_OUT | MOV_HINT,
    [FE_NOT]   = INT_IN | SAME_IN_OUT,
    [FE_NEG]   = INT_IN | SAME_IN_OUT,
    [FE_TRUNC] = INT_IN,
    [FE_SIGNEXT] = INT_IN,
    [FE_ZEROEXT] = INT_IN,
    [FE_BITCAST] = 0,
    [FE_I2F] = INT_IN,
    [FE_F2I] = FLT_IN,

    [FE_LOAD_VOLATILE] = VOL,

    [FE_STORE]          = VOL,
    [FE_STORE_UNIQUE]   = VOL,
    [FE_STORE_VOLATILE] = VOL,

    [FE_CASCADE_UNIQUE]   = VOL,
    [FE_CASCADE_VOLATILE] = VOL,

    [FE_BRANCH] = TERM,
    [FE_JUMP]   = TERM,
    [FE_RETURN] = TERM,
};

bool fe_inst_has_trait(FeInstKind kind, FeTrait trait) {
    if (kind > _FE_INST_END) return false;
    return (inst_traits[kind] & trait) != 0;
}

void fe__load_trait_table(usize start_index, FeTrait* table, usize len) {
    memcpy(&inst_traits[start_index], table, sizeof(table[0]) * len);
}