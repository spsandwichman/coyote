#include "iron.h"

static const char* ty_name[] = {
    [FE_TY_VOID] = "void",

    [FE_TY_BOOL] = "bool",

    [FE_TY_I8]  = "i8",
    [FE_TY_I16] = "i16",
    [FE_TY_I32] = "i32",
    [FE_TY_I64] = "i64",
    
    [FE_TY_F16] = "f16",
    [FE_TY_F32] = "f32",
    [FE_TY_F64] = "f64",
    
    [FE_TY_TUPLE] = "...",

    [FE_TY_I8x16] = "i8x16",
    [FE_TY_I16x8] = "i16x8",
    [FE_TY_I32x4] = "i32x4",
    [FE_TY_I64x2] = "i64x2",
    [FE_TY_F16x8] = "f16x8",
    [FE_TY_F32x4] = "f32x4",
    [FE_TY_F64x2] = "f64x2",

    [FE_TY_I8x32]  = "i8x32",
    [FE_TY_I16x16] = "i16x16",
    [FE_TY_I32x8]  = "i32x8",
    [FE_TY_I64x4]  = "i64x4",
    [FE_TY_F16x16] = "f16x16",
    [FE_TY_F32x8]  = "f32x8",
    [FE_TY_F64x4]  = "f64x4",

    [FE_TY_I8x64]  = "i8x64",
    [FE_TY_I16x32] = "i16x32",
    [FE_TY_I32x16] = "i32x16",
    [FE_TY_I64x8]  = "i64x8",
    [FE_TY_F16x32] = "f16x32",
    [FE_TY_F32x16] = "f32x16",
    [FE_TY_F64x8]  = "f64x8",
};

static const char* inst_name[_FE_BASE_INST_END] = {
    [FE_BOOKEND] = "<bookend>",

    [FE_CONST] = "const",

    [FE_PARAM] = "param",

    [FE_PROJ] = "proj",
    [FE_PROJ_VOLATILE] = "proj_volatile",

    [FE_IADD] = "iadd",
    [FE_ISUB] = "isub",
    [FE_IMUL] = "imul",
    [FE_UMUL] = "umul",
    [FE_IDIV] = "idiv",
    [FE_UDIV] = "udiv",
    [FE_IREM] = "irem",
    [FE_UREM] = "urem",
    
    [FE_AND] = "and",
    [FE_OR]  = "or",
    [FE_XOR] = "xor",
    [FE_SHL] = "shl",
    [FE_USR] = "usr",
    [FE_ISR] = "isr",
    [FE_ILT] = "ilt",
    [FE_ULT] = "ult",
    [FE_ILE] = "ile",
    [FE_ULE] = "ule",
    [FE_EQ]  = "eq",
    [FE_NE]  = "ne",

    [FE_FADD] = "fadd",
    [FE_FSUB] = "fsub",
    [FE_FSUB] = "fmul",
    [FE_FDIV] = "fdiv",
    [FE_FREM] = "frem",

    [FE_MOV] = "mov",
    [FE_MOV_VOLATILE] = "mov_volatile",
    [FE_NOT] = "not",
    [FE_NEG] = "neg",
    [FE_TRUNC] = "trunc",
    [FE_SIGNEXT] = "signext",
    [FE_ZEROEXT] = "zeroext",
    [FE_I2F] = "i2f",
    [FE_F2I] = "f2i",

    [FE_LOAD] = "load",
    [FE_LOAD_UNIQUE] = "load_unique",
    [FE_LOAD_VOLATILE] = "load_volatile",

    [FE_STORE] = "store",
    [FE_STORE_UNIQUE] = "store_unique",
    [FE_STORE_VOLATILE] = "store_volatile",

    [FE_CASCADE_UNIQUE] = "cascade_unique",
    [FE_CASCADE_VOLATILE] = "cascade_volatile",
    [FE_FAKE_SOURCE] = "fake_source",

    [FE_BRANCH] = "branch",
    [FE_JUMP] = "jump",
    [FE_RETURN] = "return",

    [FE_CALL_DIRECT] = "call_direct",
    [FE_CALL_INDIRECT] = "call_indirect",
};

// idea stolen from TB lmao
static int ansi(usize x) {
    usize hash = (x) % 11;
    if (hash >= 6) {
        return hash - 6 + 91;
    } else {
        return hash + 31;
    }
}

static bool should_ansi = true;

static void print_inst_ty(FeDataBuffer* db, FeInst* inst) {
    if (inst->ty == FE_TY_TUPLE) {
        usize index = 0;
        for (FeTy ty = fe_proj_ty(inst, index); ty != FE_TY_VOID; index++, ty = fe_proj_ty(inst, index)) {
            if (index != 0) {
                fe_db_writecstr(db, ", ");
            }
            fe_db_writecstr(db, ty_name[ty]);
        }
    } else {
        fe_db_writecstr(db, ty_name[inst->ty]);
    }
}

void fe__print_block_ref(FeDataBuffer* db, FeBlock* ref) {
    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(ref->flags));
    fe_db_writef(db, "%u:", ref->flags);
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

void fe__print_ref(FeDataBuffer* db, FeInst* ref) {
    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(ref->flags));
    fe_db_writef(db, "%%%u", ref->flags);
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

static void print_inst(FeDataBuffer* db, FeInst* inst) {

    if (inst->ty != FE_TY_VOID) {
        fe__print_ref(db, inst);
        fe_db_writef(db, ": ");
        print_inst_ty(db, inst);
        fe_db_writecstr(db, " = ");
    }

    if (inst->kind < _FE_BASE_INST_END) {
        char* name = inst_name[inst->kind];
        
        if (name) fe_db_writecstr(db, name);
        else fe_db_writef(db, "<kind %d>", inst->kind);
    } else if (fe_kind_is_xr(inst->kind)) {
        fe_db_writecstr(db, fe_xr_inst_name(inst->kind));
    }

    fe_db_writecstr(db, " ");

    switch (inst->kind) {
    case FE_IADD ... FE_FREM:
        fe__print_ref(db, fe_extra_T(inst, FeInstBinop)->lhs);
        fe_db_writecstr(db, ", ");
        fe__print_ref(db, fe_extra_T(inst, FeInstBinop)->rhs);
        break;
    case FE_MOV ... FE_F2I:
        fe__print_ref(db, fe_extra_T(inst, FeInstUnop)->un);
        break;
    case FE_PARAM:
        fe_db_writef(db, "%u", fe_extra_T(inst, FeInstParam)->index);
        break;
    case FE_RETURN:
        FeInstReturn* ret = fe_extra(inst);
        for_n(i, 0, ret->len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe__print_ref(db, fe_return_arg(inst, i));
        }
        break;
    case FE_BRANCH:
        FeInstBranch* branch = fe_extra(inst);
        fe__print_ref(db, branch->cond);
        fe_db_writecstr(db, ", ");
        fe__print_block_ref(db, branch->if_true);
        fe_db_writecstr(db, ", ");
        fe__print_block_ref(db, branch->if_false);
        break;
    case FE_CONST:
        switch (inst->ty) {
        case FE_TY_BOOL: fe_db_writef(db, "%s", fe_extra_T(inst, FeInstConst)->val ? "true" : "false"); break;
        case FE_TY_I64:  fe_db_writef(db, "%llu", fe_extra_T(inst, FeInstConst)->val); break;
        case FE_TY_I32:  fe_db_writef(db, "%llu", (u64)(u32)fe_extra_T(inst, FeInstConst)->val); break;
        case FE_TY_I16:  fe_db_writef(db, "%llu", (u64)(u16)fe_extra_T(inst, FeInstConst)->val); break;
        case FE_TY_I8:   fe_db_writef(db, "%llu", (u64)(u8)fe_extra_T(inst, FeInstConst)->val); break;
        default:
            fe_db_writef(db, "[TODO]");
            break;
        }
        break;
    case FE_CALL_DIRECT:
        FeInstCallDirect* dcall = fe_extra(inst);
        fe_db_writecstr(db, "\"");
        fe_db_write(db, dcall->to_call->sym->name, dcall->to_call->sym->name_len);
        fe_db_writecstr(db, "\", ");
        for_n(i, 0, dcall->len) {
            FeInst* arg = fe_call_arg(inst, i);
            if (i != 0) fe_db_writecstr(db, ", ");
            fe__print_ref(db, arg);
            fe_db_writecstr(db, ": ");
            fe_db_writecstr(db, ty_name[arg->ty]);
        }
        break;
    case _FE_XR_INST_BEGIN ... _FE_XR_INST_END:
        fe_xr_print_args(db, inst);
        break;
    default:
        fe_db_writef(db, "[TODO]");
    }
    fe_db_writecstr(db, "\n");
}

void fe_print_func(FeDataBuffer* db, FeFunction* func) {
    // number all instructions and blocks
    u32 inst_counter = 0;
    u32 block_counter = 0;
    for_n(i, 0, func->sig->param_len) {
        fe_func_param(func, i)->flags = inst_counter++;
    }
    for_blocks(block, func) {
        block->flags = block_counter++;
        for_inst(inst, block) {
            if (inst->ty != FE_TY_VOID)
                inst->flags = inst_counter++;
        }
    }

    switch (func->sym->bind) {
    case FE_BIND_GLOBAL: fe_db_writecstr(db, "global "); break;
    case FE_BIND_LOCAL:  fe_db_writecstr(db, "local "); break;
    case FE_BIND_SHARED_EXPORT: fe_db_writecstr(db, "shared_export "); break;
    case FE_BIND_SHARED_IMPORT: fe_runtime_crash("function cannot have shared_import binding");
    }

    // write function signature
    fe_db_writef(db, "func \"%.*s\"", func->sym->name_len, func->sym->name);
    if (func->sig->param_len) {
        fe_db_writecstr(db, " ");
        for_n(i, 0, func->sig->param_len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe__print_ref(db, fe_func_param(func, i));
            fe_db_writecstr(db, ": ");
            fe_db_writecstr(db, ty_name[fe_funcsig_param(func->sig, i)->ty]);
        }
    }
    if (func->sig->return_len) {
        fe_db_writecstr(db, " -> ");
        for_n(i, 0, func->sig->return_len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe_db_writecstr(db, ty_name[fe_funcsig_return(func->sig, i)->ty]);
        }
    }
    // write function body
    fe_db_writecstr(db, " {\n");
    for_blocks(block, func) {
        if (block != func->entry_block) {
            fe_db_writecstr(db, "  ");
            fe__print_block_ref(db, block);
            fe_db_writecstr(db, "\n");
        }
        for_inst(inst, block) {
            fe_db_writecstr(db, "    ");
            print_inst(db, inst);
        }
    }
    fe_db_writecstr(db, "}\n");
}