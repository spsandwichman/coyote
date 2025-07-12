#include "iron.h"
#include "common/util.h"
#include <string.h>

static const char* inst_name(FeInst* inst) {
    FeInst* bookend = inst;
    while (bookend->kind != FE__BOOKEND) {
        bookend = bookend->next;
    }
    const FeTarget* t = fe_extra_T(bookend, FeInst_Bookend)->block->func->mod->target;
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
    fe_free(sig->params);
    fe_free(sig);
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
        fe_inst_destroy(f, inst);
    }
}

void fe_set_input(FeFunc* f, FeInst* inst, u16 n, FeInst* input) {
    FeInst* old_input = inst->inputs[n];

    // unordered remove inst from old_input->uses
    if (old_input != nullptr) {
        old_input->uses[n] = old_input->uses[--old_input->use_len];
    }

    // add inst to input's uses
    inst->inputs[n] = input;
    if_unlikely (input->use_cap == input->use_len) {
        FeInstPool* pool = f->ipool;
        // if_unlikely (f == nullptr) {
        //     // goddamn it we didnt get an ipool
        //     // we have to traverse up the whole shit
        //     FeBlock* block = nullptr;
        //     for (FeInst* i = input; true; i = i->next) {
        //         if (i->kind != FE__BOOKEND) {
        //             block = fe_extra_T(i, FeInst_Bookend)->block;
        //             break;
        //         }
        //     }
        //     pool = block->func->ipool;
        // } else {
        //     pool = f->ipool;
        // }
        
        input->use_cap *= 2;
        // copy uses to new larger list
        FeInstUse* new_uses = fe_ipool_list_alloc(pool, input->use_cap);
        memcpy(new_uses, input->uses, sizeof(new_uses[0]) * input->use_len);
       
        // set the top list to zero
        memset(&new_uses[input->use_len], 0, sizeof(new_uses[0]) * input->use_len);
        
        // free old list
        fe_ipool_list_free(pool, input->uses, input->use_len);
        input->uses = new_uses;
    }

    input->uses[input->use_len].idx = n;
    input->uses[input->use_len].ptr = (i64)inst;
    input->use_len += 1;
}

void fe_set_input_null(FeInst* inst, u16 n) {
    FeInst* old_input = inst->inputs[n];

    // unordered remove inst from old_input->uses
    old_input->use_len -= 1;
    old_input->uses[n] = old_input->uses[old_input->use_len];

    // set current input to null
    inst->inputs[n] = nullptr;
}

void fe_cfg_add_edge(FeFunc* f, FeBlock* pred, FeBlock* succ) {
    FeInstPool* pool = f->ipool;

    // append succ to pred's succ list
    if_unlikely (pred->succ_cap == pred->succ_len) {
        pred->succ_cap *= 2;
        FeBlock** new_list = fe_ipool_list_alloc(pool, pred->succ_len);
        memcpy(new_list, pred->succ, sizeof(pred->succ[0]) * pred->succ_len);
        fe_ipool_list_free(pool, pred->succ, pred->succ_len);
        pred->succ = new_list;
    }
    pred->succ[pred->succ_len] = succ;
    pred->succ_len += 1;

    // append pred to succ's pred list
    if_unlikely (succ->pred_cap == succ->pred_len) {
        succ->pred_cap *= 2;
        FeBlock** new_list = fe_ipool_list_alloc(pool, succ->pred_len);
        memcpy(new_list, succ->pred, sizeof(succ->pred[0]) * succ->pred_len);
        fe_ipool_list_free(pool, pred->pred, succ->pred_len);
        succ->pred = new_list;
    }
    succ->pred[succ->pred_len] = pred;
    succ->pred_len += 1;
}

void fe_cfg_remove_edge(FeBlock* pred, FeBlock* succ) {
    // find succ in pred's succ list
    for_n(i, 0, pred->succ_len) {
        if (pred->succ[i] == succ) {
            // unordered remove
            pred->succ[i] = pred->succ[--pred->succ_len];
            break;
        }
    }

    // find pred in succ's pred list
    for_n(i, 0, succ->pred_len) {
        if (succ->pred[i] == pred) {
            // unordered remove
            succ->pred[i] = succ->pred[--succ->pred_len];
            break;
        } 
    }
}

static inline usize usize_next_pow_2(usize x) {
    return 1 << ((sizeof(x) * 8) -_Generic(x,
        unsigned long long: __builtin_clzll(x - 1),
        unsigned long: __builtin_clzl(x - 1),
        unsigned int: __builtin_clz(x - 1)));
}

static inline usize usize_log2(usize x) {
    return (sizeof(x) * 8 - 1) - _Generic(x,
        unsigned long long: __builtin_clzll(x),
        unsigned long: __builtin_clzl(x),
        unsigned int: __builtin_clz(x));
}

FeInst* fe_inst_new(FeFunc* f, usize input_len, usize extra_size) {
    FeInst* inst = fe_ipool_alloc(f->ipool, extra_size);
    inst->in_len = input_len;
    inst->in_cap = usize_next_pow_2(input_len);
    inst->inputs = fe_ipool_list_alloc(f->ipool, inst->in_cap);
    memset(inst->inputs, 0, sizeof(inst->inputs[0]) * inst->in_cap);

    inst->use_len = 0;
    inst->use_cap = 2;
    inst->uses = fe_ipool_list_alloc(f->ipool, inst->use_cap);
    memset(inst->uses, 0, sizeof(inst->uses[0]) * inst->use_cap);

    return inst;
}

static usize count_composite_items(FeTy ty, FeComplexTy* cty) {
    if (ty == FE_TY_ARRAY) {
        return cty->array.len * count_composite_items(
            cty->array.elem_ty, 
            cty->array.complex_elem_ty
        );
    } else if (ty == FE_TY_RECORD) {
        usize n = 0;
        for_n(i, 0, cty->record.fields_len) {
            FeRecordField* field = &cty->record.fields[i];
            n += count_composite_items(field->ty, field->complex_ty);
        }
        return n;
    }
    return 1;
}

FeFunc* fe_func_new(
    FeModule* mod,
    FeSymbol* sym,
    FeFuncSig* sig,
    FeInstPool* ipool,
    FeVRegBuffer* vregs
) {
    FeFunc* func = fe_malloc(sizeof(*func));
    memset(func, 0, sizeof(*func));
    func->ipool = ipool;
    func->mod = mod;
    func->sym = sym;
    func->vregs = vregs;

    // construct params.
    
    
    return func;
}

void fe_func_destroy(FeFunc* f) {
    // unhook from symbol
    f->sym->func = nullptr;

    // destroy blocks
    for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
        if (b->list_prev != nullptr) {
            fe_block_destroy(b->list_prev);
        }
    }
    fe_block_destroy(f->last_block);

    // destroy stack
    for (FeStackItem* s = f->stack_bottom; s != nullptr; s = s->next) {
        if (s->prev != nullptr) {
            fe_free(s->prev);
        }
    }
    fe_free(f->stack_top);

    // remove from func list
    if (f->list_next) {
        f->list_next->list_prev = f->list_prev;
    } else {
        f->mod->funcs.last = f->list_prev;
    }
    if (f->list_prev) {
        f->list_prev->list_next = f->list_next;
    } else {
        f->mod->funcs.first = f->list_next;
    }

    // free parameter inst list
    fe_free(f->params);

    // free dat func
    fe_free(f);
}

#include "short_traits.h"

static FeTrait inst_traits[FE__INST_END] = {
    [FE_PROJ] = 0,
    [FE_PARAM] = VOL,
    [FE_CONST] = 0,
    [FE_STACK_ADDR] = 0,
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
    // [FE_UPSILON]  = UNOP | VOL | SAME_IN_OUT | MOV_HINT,
    [FE_NOT]   = UNOP | INT_IN | SAME_IN_OUT,
    [FE_NEG]   = UNOP | INT_IN | SAME_IN_OUT,
    [FE_TRUNC] = UNOP | INT_IN,
    [FE_SIGN_EXT] = UNOP | INT_IN,
    [FE_ZERO_EXT] = UNOP | INT_IN,
    [FE_BITCAST] = UNOP,
    [FE_I2F] = UNOP | INT_IN,
    [FE_F2I] = UNOP | FLT_IN,
    [FE_U2F] = UNOP | INT_IN,
    [FE_F2U] = UNOP | FLT_IN,

    [FE_STORE] = VOL | MEM_USE | MEM_DEF,
    [FE_MEM_BARRIER] = VOL | MEM_USE | MEM_DEF,
    [FE_LOAD] = MEM_USE,
    [FE_CALL] = MEM_USE | MEM_DEF,

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
