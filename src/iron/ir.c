#include "iron.h"
#include "iron/iron.h"
#include <string.h>

static const char* inst_name(FeInst* inst) {
    FeInst* bookend = inst;
    while (bookend->kind != FE__BOOKEND) {
        bookend = bookend->next;
    }
    const FeTarget* t = fe_extra_T(bookend, FeInst__Bookend)->block->func->mod->target;
    return fe_inst_name(t, inst->kind);
}

FeModule* fe_module_new(FeArch arch, FeSystem system) {
    FeModule* mod = fe_malloc(sizeof(*mod));
    memset(mod, 0, sizeof(*mod));

    mod->target = fe_make_target(arch, system);
    fe_symtab_init(&mod->symtab);
    return mod;
}

void fe_module_destroy(FeModule* mod) {
    // destroy the functions
    while (mod->funcs.first) {
        fe_func_destroy(mod->funcs.first);  
    }

    fe_free((void*)mod->target);
    fe_free(mod);
}

// void fe_section_destroy(FeSection* section) {

// }

FeSection* fe_section_new(FeModule* m, const char* name, u16 len, FeSectionFlags flags) {
    FeSection* section = fe_malloc(sizeof(*section));
    memset(section, 0, sizeof(*section));

    if (len == 0) {
        len = strlen(name);
    }

    section->name = fe_compstr(name, len);
    section->flags = flags;

    if (m->sections.first == nullptr) {
        m->sections.first = section;
        m->sections.last = section;
    } else {
        section->prev = m->sections.last;
        m->sections.last->next = section;
        m->sections.last = section;
    }

    return section;
}

FeComplexTy* fe_ty_record_new(u32 fields_len) {
    FeComplexTy* record = fe_malloc(sizeof(*record));
    record->ty = FE_TY_RECORD;
    record->record.fields_len = fields_len;
    record->record.fields = fe_malloc(sizeof(FeRecordField) * fields_len);

    return record;
}

// if elem_ty is not a complex type, elem_cty should be nullptr
FeComplexTy* fe_ty_array_new(u32 array_len, FeTy elem_ty, FeComplexTy* elem_cty) {
    FeComplexTy* array = fe_malloc(sizeof(*array));
    array->ty = FE_TY_ARRAY;
    array->array.elem_ty = elem_ty;
    array->array.complex_elem_ty = elem_cty;
    array->array.len = array_len;
    return array;
}

usize fe_ty_get_align(FeTy ty, FeComplexTy *cty) {
    switch (ty) {
    case FE_TY_VOID:
        FE_CRASH("cant get align of void");
    case FE_TY_TUPLE:
        FE_CRASH("cant get align of tuple");
    case FE_TY_ARRAY:
        if (cty != nullptr) {
            FE_CRASH("ty is array but no FeComplexTy was provided");
        }
        return fe_ty_get_align(cty->array.elem_ty, cty->array.complex_elem_ty);
    case FE_TY_RECORD: {
        if (cty != nullptr) {
            FE_CRASH("ty is record but no FeComplexTy was provided");
        }
        usize align = 1;
        FeRecordField* fields = cty->record.fields;
        for_n(i, 0, cty->record.fields_len) {
            usize field_align = fe_ty_get_align(fields[i].ty, fields[i].complex_ty);
            if (align < field_align) {
                align = field_align;
            }
        }
        return align;
    }
    case FE_TY_BOOL:
    case FE_TY_I8:
        return 1;
    case FE_TY_I16:
    case FE_TY_F16:
        return 2;
    case FE_TY_I32:
    case FE_TY_F32:
        return 4;
    case FE_TY_I64:
    case FE_TY_F64:
        return 8;
    default:
        if (FE_TY_IS_VEC(ty)) {
            switch (FE_TY_VEC_SIZETY(ty)) {
            case FE_TY_V128:
                return 16;
            case FE_TY_V256:
                return 32;
            case FE_TY_V512:
                return 64;
            default:
                FE_CRASH("insane vector type");
            }
        }
        FE_CRASH("unknown ty %u", ty);
    }
}

usize fe_ty_get_size(FeTy ty, FeComplexTy *cty) {
    switch (ty) {
    case FE_TY_VOID:
        FE_CRASH("cant get size of void");
    case FE_TY_TUPLE:
        FE_CRASH("cant get size of tuple");
    case FE_TY_ARRAY:
        if (cty == nullptr) {
            FE_CRASH("ty is array but no FeComplexTy was provided");
        }
        return cty->array.len * fe_ty_get_size(cty->array.elem_ty, cty->array.complex_elem_ty);
    case FE_TY_RECORD: {
        if (cty == nullptr) {
            FE_CRASH("ty is record but no FeComplexTy was provided");
        }

        usize align = fe_ty_get_align(ty, cty);

        FeRecordField* last_field = &cty->record.fields[cty->record.fields_len];
        return last_field->offset + last_field->offset;
    }
    case FE_TY_BOOL:
    case FE_TY_I8:
        return 1;
    case FE_TY_I16:
    case FE_TY_F16:
        return 2;
    case FE_TY_I32:
    case FE_TY_F32:
        return 4;
    case FE_TY_I64:
    case FE_TY_F64:
        return 8;
    default:
        if (FE_TY_IS_VEC(ty)) {
            switch (FE_TY_VEC_SIZETY(ty)) {
            case FE_TY_V128:
                return 16;
            case FE_TY_V256:
                return 32;
            case FE_TY_V512:
                return 64;
            default:
                FE_CRASH("insane vector type");
            }
        }
        FE_CRASH("unknown ty %u", ty);
    }
}

// if len == 0, calculate with strlen
FeSymbol* fe_symbol_new(FeModule* m, const char* name, u16 len, FeSection* section, FeSymbolBinding bind) {
    if (name == nullptr) {
        FE_CRASH("symbol cannot be null");
    }
    if (len == 0) {
        len = strlen(name);
    }
    if (len == 0) {
        FE_CRASH("symbol cannot have zero length");
    }
    // put this in an arena of some sort later
    FeSymbol* sym = fe_malloc(sizeof(FeSymbol));
    sym->kind = FE_SYMKIND_NONE;
    sym->bind = bind;
    sym->section = section;
    sym->name = fe_compstr(name, len);

    if (fe_symtab_get(&m->symtab, name, len)) {
        FE_CRASH("symbol with name '%.*s' already exists", len, name);
    }
    fe_symtab_put(&m->symtab, sym);

    return sym;
}

void fe_symbol_destroy(FeSymbol* sym) {
    memset(sym, 0, sizeof(*sym));
    fe_free(sym);
}

FeFuncSig* fe_funcsig_new(FeCallConv cconv, u16 param_len, u16 return_len) {
    FeFuncSig* sig = fe_malloc(
        sizeof(*sig) + (param_len + return_len) * sizeof(sig->params[0])
    );
    sig->cconv = cconv,
    sig->param_len = param_len;
    sig->return_len = return_len;
    return sig;
};

FeFuncParam* fe_funcsig_param(FeFuncSig* sig, u16 index) {
    if (index >= sig->param_len) {
        FE_CRASH("index > param_len");
    }
    return &sig->params[index];
}

FeFuncParam* fe_funcsig_return(FeFuncSig* sig, u16 index) {
    if (index >= sig->return_len) {
        FE_CRASH("index > return_len");
    }
    return &sig->params[sig->param_len + index];
}

void fe_funcsig_destroy(FeFuncSig* sig) {
    fe_free(sig);
}

FeBlock* fe_block_new(FeFunc* f) {
    FeBlock* block = fe_malloc(sizeof(*block));
    memset(block, 0, sizeof(*block));

    block->func = f;
    
    // adds initial bookend instruction to block
    FeInst* bookend = fe_ipool_alloc(f->ipool, sizeof(FeInst__Bookend));
    bookend->kind = FE__BOOKEND;
    bookend->ty = FE_TY_VOID;
    bookend->next = bookend;
    bookend->prev = bookend;
    fe_extra_T(bookend, FeInst__Bookend)->block = block;
    block->bookend = bookend;

    // append to block list
    if (f->last_block) {
        FeBlock* last = f->last_block;
        block->list_prev = last;
        last->list_next = block;
        f->last_block = block;
    } else {
        f->last_block = block;
        f->entry_block = block;
    }
    return block;
}

void fe_block_destroy(FeBlock *block) {
    FeFunc* f = block->func;

    // remove from linked list
    if (block->list_next) {
        block->list_next->list_prev = block->list_prev;
    } else {
        f->last_block = block->list_prev;
    }
    if (block->list_prev) {
        block->list_prev->list_next = block->list_next;
    } else {
        f->entry_block = block->list_next;
    }

    for_inst(inst, block) {
        fe_inst_free(f, fe_inst_remove_pos(inst));
    }
    fe_inst_free(f, block->bookend);
    fe_free(block);
}

FeInstChain fe_chain_from_block(FeBlock* block) {
    FeInstChain chain;
    
    // extract it from the block's inst list
    chain.begin = block->bookend->next;
    chain.end = block->bookend->prev;
    chain.begin->prev = nullptr;
    chain.end->next = nullptr;
    
    // remove it from the block
    block->bookend->next = block->bookend;
    block->bookend->prev = block->bookend;
    return chain;
}

FeFunc* fe_func_new(FeModule* mod, FeSymbol* sym, FeFuncSig* sig, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeFunc* f = fe_malloc(sizeof(FeFunc));
    memset(f, 0, sizeof(*f));
    f->sig = sig;
    f->mod = mod;
    f->ipool = ipool;
    f->vregs = vregs;
    f->sym = sym;
    sym->func = f;
    sym->kind = FE_SYMKIND_FUNC;

    // append to function list
    if (mod->funcs.first == nullptr) {
        mod->funcs.first = f;
        mod->funcs.last = f;
    } else {
        f->list_prev = mod->funcs.last;
        f->list_prev->list_next = f;
        mod->funcs.last = f;
    }
    
    // add initial basic block
    f->entry_block = f->last_block = fe_block_new(f);

    f->params = fe_malloc(sizeof(f->params[0]) * sig->param_len);

    // adds parameter instructions
    for_n(i, 0, sig->param_len) {
        FeInst* param = fe_ipool_alloc(ipool, sizeof(FeInstParam));
        param->prev = nullptr;
        param->next = nullptr;
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

void fe_func_destroy(FeFunc *f) {
    if (f->params) {
        fe_free(f->params);    
    }

    // free the block list
    while (f->entry_block) {
        fe_block_destroy(f->entry_block);
    }
    
    // free the stack
    while (f->stack_top) {
        fe_free(fe_stack_remove(f, f->stack_top));
    }

    // remove from linked list
    if (f->list_next == nullptr) { // at back of list
        f->mod->funcs.last = f->list_prev;
    } else {
        f->list_next->list_prev = f->list_prev;
    }
    if (f->list_prev == nullptr) { // at front of list
        f->mod->funcs.first = f->list_next;
    } else {
        f->list_prev->list_next = f->list_next;
    }
    // f->list_next = NULL;
    // f->list_prev = NULL;
    fe_free(f);
}

FeInst* fe_func_param(FeFunc* f, u16 index) {
    return f->params[index];
}

void fe_inst_add_use(FeInst* def, FeInst* use) {
    if (def->uses == nullptr) {
        def->use_len = 1;
        def->use_cap = 8;
        def->uses = fe_malloc(sizeof(def->uses[0]) * def->use_cap);
        def->uses[0] = use;
        return;
    }
    if (def->use_cap == def->use_len) {
        def->use_cap *= 2;
        def->uses = fe_realloc(def->uses, sizeof(def->uses[0]) * def->use_cap);
    }
    def->uses[def->use_len] = use;
    def->use_len += 1;
}

void fe_inst_unordered_remove_use(FeInst* def, FeInst* use) {
    for_n(i, 0, def->use_len) {
        if (def->uses[i] == use) {
            def->use_len -= 1;
            def->uses[i] = def->uses[def->use_len];
            break;
        }
    }
}

void fe_inst_calculate_uses(FeFunc* f) {
    const FeTarget* t = f->mod->target;

    for_blocks(block, f) {
        for_inst(inst, block) {
            inst->use_len = 0;
        }
    }
    for_blocks(block, f) {
        for_inst(inst, block) {
            usize len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &len);
            for_n (i, 0, len) {
                fe_inst_add_use(inputs[i], inst);
            }
        }
    }
}

// likely to break; too bad!
FeInst** fe_inst_list_inputs(const FeTarget* t, FeInst* inst, usize* len_out) {
    if (inst->kind > FE__BASE_INST_END) {
        return t->list_inputs(inst, len_out);
    }

    switch (inst->kind) {
    case FE_PROJ:
    case FE__MACH_PROJ:
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
    case FE_CALL:
        ;
        FeInstCall* call = fe_extra(inst);
        *len_out = call->len + 1;
        if (call->cap == 0) {
            return &call->single.callee;
        } else {
            return call->multi;
        }
    case FE_RETURN:
        ;
        FeInstReturn* ret = fe_extra(inst);
        *len_out = ret->len;
        if (ret->cap == 0) {
            return &ret->single;
        } else {
            return ret->multi;
        }
    case FE_PHI:
        *len_out = fe_extra_T(inst, FeInstPhi)->len;
        return fe_extra_T(inst, FeInstPhi)->vals;
    case FE__MACH_STACK_SPILL:
        *len_out = 1;
        return &fe_extra_T(inst, FeInst__MachStackSpill)->val;
    case FE__BOOKEND:
    case FE_PARAM:
    case FE_CONST:
    case FE_SYM_ADDR:
    case FE_STACK_ADDR:
    case FE_CASCADE_VOLATILE:
    case FE_JUMP:
    case FE__MACH_REG:
    case FE__MACH_STACK_RELOAD:
        *len_out = 0;
        return nullptr;
    default:
        FE_CRASH("unknown kind %d", inst->kind);
        break;
    }
}

FeBlock** fe_inst_list_terminator_successors(const FeTarget* t, FeInst* term, usize* len_out) {
    if (!fe_inst_has_trait(term->kind, FE_TRAIT_TERMINATOR)) {
        FE_CRASH("list_targets: inst %s is not a terminator", inst_name(term));
    }
    if (term->kind > FE__BASE_INST_END) {
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
        return nullptr;
    }
}

void fe_inst_free(FeFunc* f, FeInst* inst) {
    if (inst->kind == FE_CALL && fe_extra_T(inst, FeInstCall)->cap != 0) {
        fe_free(fe_extra_T(inst, FeInstCall)->multi);
    }
    if (inst->kind == FE_RETURN && fe_extra_T(inst, FeInstReturn)->cap != 0) {
        fe_free(fe_extra_T(inst, FeInstReturn)->multi);
    }
    if (inst->kind == FE_PHI) {
        fe_free(fe_extra_T(inst, FeInstPhi)->blocks);
        fe_free(fe_extra_T(inst, FeInstPhi)->vals);
    }
    if (inst->uses != nullptr) {
        fe_free(inst->uses);
        inst->uses = nullptr;
    }
    fe_ipool_free(f->ipool, inst);
}

FeInst* fe_inst_remove_pos(FeInst* inst) {
    inst->next->prev = inst->prev;
    inst->prev->next = inst->next;
    inst->next = nullptr;
    inst->prev = nullptr;
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

FeInstChain fe_chain_new(FeInst* initial) {
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

void fe_chain_destroy(FeFunc* f, FeInstChain chain) {
    for (FeInst* inst = chain.begin, *next = inst->next; inst == nullptr; inst = next, next = next->next) {
        fe_inst_free(f, inst);
    }
}

FeInst* fe_inst_const(FeFunc* f, FeTy ty, u64 val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstConst));
    inst->kind = FE_CONST;
    inst->ty = ty;
    fe_extra_T(inst, FeInstConst)->val = val;
    return inst;
}

FeInst* fe_inst_const_f64(FeFunc* f, f64 val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstConst));
    inst->kind = FE_CONST;
    inst->ty = FE_TY_F64;
    fe_extra_T(inst, FeInstConst)->val_f64 = val;
    return inst;
}

FeInst* fe_inst_const_f32(FeFunc* f, f32 val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstConst));
    inst->kind = FE_CONST;
    inst->ty = FE_TY_F32;
    fe_extra_T(inst, FeInstConst)->val_f32 = val;
    return inst;
}

FeInst* fe_inst_const_f16(FeFunc* f, f16 val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstConst));
    inst->kind = FE_CONST;
    inst->ty = FE_TY_F16;
    fe_extra_T(inst, FeInstConst)->val_f16 = val;
    return inst;
}

FeInst* fe_inst_stack_addr(FeFunc* f, FeTy ty, FeStackItem* item) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstStackAddr));
    inst->kind = FE_STACK_ADDR;
    inst->ty = ty;
    fe_extra_T(inst, FeInstStackAddr)->item = item;
    return inst;
}

FeInst* fe_inst_sym_addr(FeFunc* f, FeTy ty, FeSymbol* sym) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstSymAddr));
    inst->kind = FE_SYM_ADDR;
    inst->ty = ty;
    fe_extra_T(inst, FeInstSymAddr)->sym = sym;
    return inst;
}

FeInst* fe_inst_unop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* val) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstUnop));
    inst->kind = kind;
    inst->ty = ty;
    fe_extra_T(inst, FeInstUnop)->un = val;
    return inst;
}

FeInst* fe_inst_binop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* lhs, FeInst* rhs) {
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

FeInst* fe_inst_bare(FeFunc* f, FeTy ty, FeInstKind kind) {
    FeInst* inst = fe_ipool_alloc(f->ipool, 0);
    inst->kind = kind;
    inst->ty = ty;
    return inst;
}

FeInst* fe_inst_return(FeFunc* f) {
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

FeInst* fe_return_arg(FeInst* ret, u16 index) {
    FeInstReturn* r = fe_extra_T(ret, FeInstReturn);
    if (index >= r->len) {
        FE_CRASH("index >= ret->len");
    }
    if (r->cap == 0) {
        return r->single;
    } else {
        return r->multi[index];
    }
}

void fe_return_set_arg(FeInst* ret, u16 index, FeInst* arg) {
    FeInstReturn* r = fe_extra_T(ret, FeInstReturn);
    if (index >= r->len) {
        FE_CRASH("index >= ret->len");
    }
    
    // arg->use_len++; // since we dont do it earlier...

    if (r->cap == 0) {
        r->single = arg;
    } else {
        r->multi[index] = arg;
    }
}

FeInst* fe_inst_branch(FeFunc* f, FeInst* cond, FeBlock* if_true, FeBlock* if_false) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstBranch));
    inst->kind = FE_BRANCH;
    FeInstBranch* branch = fe_extra(inst);
    branch->cond = cond;
    branch->if_true = if_true;
    branch->if_false = if_false;
    return inst;
}

FeInst* fe_inst_jump(FeFunc* f, FeBlock* to) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstJump));
    inst->kind = FE_JUMP;
    FeInstJump* jump = fe_extra(inst);
    jump->to = to;
    return inst;
}

FeInst* fe_inst_phi(FeFunc* f, FeTy ty, u16 num_srcs) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstPhi));
    inst->kind = FE_PHI;
    inst->ty = ty;
    FeInstPhi* phi = fe_extra(inst);
    phi->len = num_srcs;
    phi->cap = num_srcs;
    phi->vals = fe_malloc(sizeof(phi->vals[0]) * num_srcs);
    phi->blocks = fe_malloc(sizeof(phi->blocks[0]) * num_srcs);
    return inst;
}

FeInst* fe_phi_get_src_val(FeInst* inst, u16 index) {
    FeInstPhi* phi = fe_extra(inst);
    if (index >= phi->len) {
        FE_CRASH("phi src index is out of bounds [0, %u)", index);
    }
    return phi->vals[index];
}

FeBlock* fe_phi_get_src_block(FeInst* inst, u16 index) {
    FeInstPhi* phi = fe_extra(inst);
    if (index >= phi->len) {
        FE_CRASH("phi src index is out of bounds [0, %u)", index);
    }
    return phi->blocks[index];
}

void fe_phi_set_src(FeInst* inst, u16 index, FeInst* val, FeBlock* block) {
    FeInstPhi* phi = fe_extra(inst);
    if (index >= phi->len) {
        FE_CRASH("phi src index is out of bounds [0, %u)", index);
    }
    phi->vals[index] = val;
    phi->blocks[index] = block;
}

void fe_phi_append_src(FeInst* inst, FeInst* val, FeBlock* block) {
    FeInstPhi* phi = fe_extra(inst);
    if (phi->len == phi->cap) {
        phi->cap += phi->cap >> 1;
        phi->vals = fe_realloc(phi->vals, sizeof(phi->vals[0]) * phi->cap);
        phi->blocks = fe_realloc(phi->blocks, sizeof(phi->blocks[0]) * phi->cap);
    }
    phi->vals[phi->len] = val;
    phi->blocks[phi->len] = block;
    phi->len += 1;
}

void fe_phi_remove_src_unordered(FeInst* inst, u16 index) {
    FeInstPhi* phi = fe_extra(inst);
    if (index >= phi->len) {
        FE_CRASH("phi src index is out of bounds [0, %u)", index);
    }
    if (index != phi->len - 1) {
        phi->vals[index] = phi->vals[phi->len - 1];
        phi->blocks[index] = phi->blocks[phi->len - 1];
    }
    phi->len -= 1;
}

FeInst* fe_inst_call(FeFunc* f, FeInst* callee, FeFuncSig* sig) {
    FeInst* inst = fe_ipool_alloc(f->ipool, sizeof(FeInstCall));
    inst->kind = FE_CALL;
    FeInstCall* call = fe_extra(inst);
    call->sig = sig;

    // set up parameters
    usize num_params = sig->param_len;
    call->len = num_params;
    if (num_params < 2) {
        call->cap = 0;
        call->single.callee = callee;
    } else {
        call->cap = num_params;
        // allocate buffer
        call->multi = fe_malloc(sizeof(FeInst*) * (num_params + 1));
        call->multi[0] = callee;
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

FeInst* fe_call_indirect_callee(FeInst* call) {
    FeInstCall* c = fe_extra(call);
    if (c->cap == 0) {
        return c->single.callee;
    } else {
        return c->multi[0];
    }
}

void fe_call_indirect_set_callee(FeInst* call, FeInst* callee) {
    FeInstCall* c = fe_extra(call);
    if (c->cap == 0) {
        c->single.callee = callee;
    } else {
        c->multi[0] = callee;
    }
}

FeInst* fe_call_arg(FeInst* call, u16 index) {
    FeInstCall* c = fe_extra(call);
    if (index >= c->len) {
        FE_CRASH("index >= ret->len");
    }
    if (c->cap == 0) {
        return c->single.arg;
    } else {
        return c->multi[index + 1]; // offset for callee
    }
}

void fe_call_set_arg(FeInst* call, u16 index, FeInst* arg) {
    FeInstCall* c = fe_extra(call);
    if (index >= c->len) {
        FE_CRASH("index >= ret->len");
    }
    if (c->cap == 0) {
        c->single.arg = arg;
    } else {
        c->multi[index + 1] = arg; // offset for callee
    }
}

FeTy fe_proj_ty(FeInst* tuple, usize index) {
    if (tuple->ty != FE_TY_TUPLE) {
        FE_CRASH("projection on non-tuple inst %s", inst_name(tuple));
    }
    switch (tuple->kind) {
    case FE_CALL:
        ;
        FeInstCall* icall = fe_extra(tuple);
        if (index < icall->sig->return_len) {
            return fe_funcsig_return(icall->sig, index)->ty;
        }
        FE_CRASH("index %zu out of bounds for [0, %u)", icall->sig->return_len);
    default:
        FE_CRASH("unknown inst kind %u", tuple->kind);
    }
}

#include "short_traits.h"

static FeTrait inst_traits[FE__INST_END] = {
    [FE_PROJ] = 0,
    [FE__MACH_PROJ] = VOL,
    [FE_PARAM] = VOL,
    [FE_CONST] = 0,
    [FE_SYM_ADDR] = 0,

    [FE_IADD] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_ISUB] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_IMUL] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_IDIV] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_UDIV] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_IREM] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_UREM] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_AND]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_OR]   = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_XOR]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_SHL]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_USR]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_ISR]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_ILT]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_ULT]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_ILE]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_ULE]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_IEQ]  = BINOP | INT_IN | VEC_IN | SAME_INS | BOOL_OUT | COMMU,

    [FE_FADD] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | FAST_ASSOC,
    [FE_FSUB] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_FMUL] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | FAST_ASSOC,
    [FE_FDIV] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_FREM] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,

    [FE_MOV]      = UNOP | SAME_IN_OUT,
    [FE__MACH_MOV] = UNOP | VOL | SAME_IN_OUT | MOV_HINT,
    [FE_UPSILON]  = UNOP | VOL | SAME_IN_OUT | MOV_HINT,
    [FE_NOT]   = UNOP | INT_IN | SAME_IN_OUT,
    [FE_NEG]   = UNOP | INT_IN | SAME_IN_OUT,
    [FE_TRUNC] = UNOP | INT_IN,
    [FE_SIGN_EXT] = UNOP | INT_IN,
    [FE_ZERO_EXT] = UNOP | INT_IN,
    [FE_BITCAST] = UNOP | 0,
    [FE_I2F] = UNOP | INT_IN,
    [FE_F2I] = UNOP | FLT_IN,
    [FE_U2F] = UNOP | INT_IN,
    [FE_F2U] = UNOP | FLT_IN,

    [FE_LOAD_VOLATILE] = VOL,

    [FE_STORE]          = VOL,
    [FE_STORE_VOLATILE] = VOL,

    [FE_CASCADE_VOLATILE] = VOL,

    [FE_BRANCH] = TERM | VOL,
    [FE_JUMP]   = TERM | VOL,
    [FE_RETURN] = TERM | VOL,
};

FeTrait fe_inst_traits(FeInstKind kind) {
    if (kind > FE__INST_END) return 0;
    return inst_traits[kind];
}

bool fe_inst_has_trait(FeInstKind kind, FeTrait trait) {
    if (kind > FE__INST_END) return false;
    return (inst_traits[kind] & trait) != 0;
}

void fe__load_trait_table(usize start_index, FeTrait* table, usize len) {
    memcpy(&inst_traits[start_index], table, sizeof(table[0]) * len);
}

void fe_wl_init(FeWorklist* wl) {
    wl->cap = 256;
    wl->len = 0;
    wl->at = fe_malloc(sizeof(wl->at[0]) * wl->cap);
}

void fe_wl_push(FeWorklist* wl, FeInst* inst) {
    if (wl->len == wl->cap) {
        wl->cap += wl->cap >> 1;
        wl->at = fe_realloc(wl->at, sizeof(wl->at[0]) * wl->cap);
    }
    wl->at[wl->len++] = inst;
}

FeInst* fe_wl_pop(FeWorklist* wl) {
    return wl->at[--wl->len];
}

void fe_wl_destroy(FeWorklist* wl) {
    fe_free(wl->at);
    *wl = (FeWorklist){0};
}
