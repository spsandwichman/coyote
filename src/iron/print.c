#include <stdlib.h>
#include <string.h>

#include "iron.h"
#include "iron/iron.h"

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

    [FE_TY_TUPLE]  = "...",
    [FE_TY_RECORD] = "{...}",
    [FE_TY_ARRAY]  = "[...]",

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

static const char* inst_name[FE__BASE_INST_END] = {
    [FE__BOOKEND] = "<bookend>",

    [FE_CONST] = "const",

    [FE_STACK_ADDR] = "stack-addr",
    [FE_SYM_ADDR] = "sym-addr",

    [FE_PARAM] = "param",

    [FE_PROJ] = "proj",
    [FE__MACH_PROJ] = "mach-proj",

    [FE_IADD] = "iadd",
    [FE_ISUB] = "isub",
    [FE_IMUL] = "imul",
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
    [FE_IEQ] = "ieq",

    [FE_FADD] = "fadd",
    [FE_FSUB] = "fsub",
    [FE_FMUL] = "fmul",
    [FE_FDIV] = "fdiv",
    [FE_FREM] = "frem",

    [FE_MOV] = "mov",
    [FE__MACH_MOV] = "mach-mov",
    [FE_UPSILON] = "upsilon",
    [FE_NOT] = "not",
    [FE_NEG] = "neg",
    [FE_TRUNC] = "trunc",
    [FE_SIGN_EXT] = "sign-ext",
    [FE_ZERO_EXT] = "zero-ext",
    [FE_I2F] = "i2f",
    [FE_F2I] = "f2i",
    [FE_U2F] = "u2f",
    [FE_F2U] = "f2u",

    [FE_LOAD] = "load",
    [FE_LOAD_VOLATILE] = "load-vol",

    [FE_STORE] = "store",
    [FE_STORE_VOLATILE] = "store-vol",

    [FE_CASCADE_VOLATILE] = "cascade-vol",
    [FE__MACH_REG] = "mach-reg",

    [FE_BRANCH] = "branch",
    [FE_JUMP] = "jump",
    [FE_RETURN] = "return",

    [FE_PHI] = "phi",

    [FE_CALL] = "call",
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

thread_local static bool should_ansi = true;

static void print_ty(FeDataBuffer* db, FeTy ty, FeComplexTy* cty) {
    if (ty == FE_TY_RECORD) {
        if (cty != nullptr) {
            FE_CRASH("ty is record but no FeComplexTy was provided");
        }
        fe_db_writecstr(db, "{ ");

        for_n (i, 0, cty->record.fields_len) {
            FeRecordField* field = &cty->record.fields[i];
            if (i != 0) {
                fe_db_writecstr(db, ", ");
            }
            fe_db_writef(db, "%u: ", field->offset);
            print_ty(db, field->ty, field->complex_ty);
        }

        fe_db_writecstr(db, " }");
    } else if (ty == FE_TY_ARRAY) {
        if (cty != nullptr) {
            FE_CRASH("ty is array but no FeComplexTy was provided");
        }

        fe_db_writecstr(db, "[");
        fe_db_writef(db, "%u * ", cty->array.len);
        print_ty(db, cty->array.elem_ty, cty->array.complex_elem_ty);
        fe_db_writecstr(db, "]");
    } else {
        fe_db_writecstr(db, ty_name[ty]);
    }
}

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

void fe__emit_ir_stack_label(FeDataBuffer* db, FeStackItem* item) {
    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(item->flags));
    fe_db_writef(db, "s%u", item->flags);
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

void fe__emit_ir_block_label(FeDataBuffer* db, FeFunc* f, FeBlock* ref) {
    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(ref->flags));
    fe_db_writef(db, "%u:", ref->flags);
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

void fe__emit_ir_ref(FeDataBuffer* db, FeFunc* f, FeInst* ref) {
    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(ref->flags));
    fe_db_writef(db, "%%%u", ref->flags);
    if (ref->vr_out != FE_VREG_NONE) {
        // BAD ASSUMPTION
        if (fe_vreg(f->vregs, ref->vr_out)->real == FE_VREG_REAL_UNASSIGNED) {
            fe_db_writef(db, "(vr%u)", ref->vr_out);
        } else {
            FeVirtualReg* vr = fe_vreg(f->vregs, ref->vr_out);
            fe_db_writef(db, "(%s)", f->mod->target->reg_name(vr->class, vr->real));
        }
    }
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

const char* fe_inst_name(const FeTarget* target, FeInstKind kind) {
    if (kind < FE__BASE_INST_END) {
        return inst_name[kind];
    } else {
        return target->inst_name(kind, true);
    }
}

const char* fe_ty_name(FeTy ty) {
    if (ty < FE__TY_END) {
        return ty_name[ty];
    }
    return nullptr;
}


static void print_inst(FeFunc* f, FeDataBuffer* db, FeInst* inst) {
    const FeTarget* target = f->mod->target;

    if (inst->ty != FE_TY_VOID) {
        fe__emit_ir_ref(db, f, inst);
        fe_db_writef(db, ": ");
        print_inst_ty(db, inst);
        fe_db_writecstr(db, " = ");
    }

    if (inst->kind < FE__BASE_INST_END) {
        const char* name = inst_name[inst->kind];
        
        if (name) fe_db_writecstr(db, name);
        else fe_db_writef(db, "<kind %d>", inst->kind);
    } else {
        fe_db_writecstr(db, target->inst_name(inst->kind, true));
        
    }

    fe_db_writecstr(db, " ");

    switch (inst->kind) {
    case FE_IADD ... FE_FREM:
        fe__emit_ir_ref(db, f, fe_extra_T(inst, FeInstBinop)->lhs);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, fe_extra_T(inst, FeInstBinop)->rhs);
        break;
    case FE_MOV ... FE_F2U:
        fe__emit_ir_ref(db, f, fe_extra_T(inst, FeInstUnop)->un);
        break;
    case FE_PARAM:
        fe_db_writef(db, "%u", fe_extra_T(inst, FeInstParam)->index);
        break;
    case FE_RETURN:
        ;
        FeInstReturn* ret = fe_extra(inst);
        for_n(i, 0, ret->len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe__emit_ir_ref(db, f, fe_return_arg(inst, i));
        }
        break;
    case FE_CALL:
        ;
        FeInstCall* call = fe_extra(inst);
        if (call->cap == 0) {
            fe__emit_ir_ref(db, f, call->single.callee);
            fe_db_writecstr(db, " (");
            if (call->len != 0) {
                fe__emit_ir_ref(db, f, call->single.arg);
                fe_db_writecstr(db, ": ");
                fe_db_writecstr(db, fe_ty_name(call->single.arg->ty));
            }
            fe_db_writecstr(db, ")");
        } else {
            fe__emit_ir_ref(db, f, call->multi[0]);
            fe_db_writecstr(db, " (");
            for_n(i, 0, call->len) {
                if (i != 0) fe_db_writecstr(db, ", ");
                FeInst* arg = fe_call_arg(inst, i);
                fe__emit_ir_ref(db, f, arg);
                fe_db_writecstr(db, ": ");
                fe_db_writecstr(db, fe_ty_name(arg->ty));
            }
            fe_db_writecstr(db, ")");
        }

        break;
    case FE_BRANCH:
        ;
        FeInstBranch* branch = fe_extra(inst);
        fe__emit_ir_ref(db, f, branch->cond);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_block_label(db, f, branch->if_true);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_block_label(db, f, branch->if_false);
        break;
    case FE_JUMP:
        ;
        FeInstJump* jump = fe_extra(inst);
        fe__emit_ir_block_label(db, f, jump->to);
        break;
    case FE_PHI:
        ;
        FeInstPhi* phi = fe_extra(inst);
        for_n(i, 0, phi->len) {
            if (i != 0) {
                fe_db_writecstr(db, ", ");
            }
            FeBlock* src_block = phi->blocks[i];
            FeInst* src = phi->vals[i];
            fe__emit_ir_block_label(db, f, src_block);
            fe_db_writecstr(db, " ");
            fe__emit_ir_ref(db, f, src);
        }
        break;
    case FE_SYM_ADDR:
        ;
        FeSymbol* sym = fe_extra_T(inst, FeInstSymAddr)->sym;
        fe_db_writecstr(db, "\"");
        fe_db_write(db, fe_compstr_data(sym->name), sym->name.len);
        fe_db_writecstr(db, "\"");
        break;
    case FE_STACK_ADDR:
        ;
        FeStackItem* item = fe_extra_T(inst, FeInstStackAddr)->item;
        fe__emit_ir_stack_label(db, item);
        break;
    case FE_LOAD:
    case FE_LOAD_VOLATILE:
        ;
        FeInstLoad* load = fe_extra(inst);
        if (load->unaligned) {
            fe_db_writecstr(db, "unaligned ");
        }
        fe__emit_ir_ref(db, f, load->ptr);
        break;
    case FE_STORE:
    case FE_STORE_VOLATILE:
        ;
        FeInstStore* store = fe_extra(inst);
        if (store->unaligned) {
            fe_db_writecstr(db, "unaligned ");
        }
        fe__emit_ir_ref(db, f, store->ptr);
            fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, store->val);
        break;
    case FE_CONST:
        switch (inst->ty) {
        case FE_TY_BOOL: fe_db_writef(db, "%s", fe_extra_T(inst, FeInstConst)->val ? "true" : "false"); break;
        case FE_TY_F64:  fe_db_writef(db, "%lf", (f64)fe_extra_T(inst, FeInstConst)->val_f64); break;
        case FE_TY_F32:  fe_db_writef(db, "%lf", (f64)fe_extra_T(inst, FeInstConst)->val_f32); break;
        case FE_TY_F16:  fe_db_writef(db, "%lf", (f64)fe_extra_T(inst, FeInstConst)->val_f16); break;
        case FE_TY_I64:  fe_db_writef(db, "0x%llx", (u64)fe_extra_T(inst, FeInstConst)->val); break;
        case FE_TY_I32:  fe_db_writef(db, "0x%llx", (u64)(u32)fe_extra_T(inst, FeInstConst)->val); break;
        case FE_TY_I16:  fe_db_writef(db, "0x%llx", (u64)(u16)fe_extra_T(inst, FeInstConst)->val); break;
        case FE_TY_I8:   fe_db_writef(db, "0x%llx", (u64)(u8)fe_extra_T(inst, FeInstConst)->val); break;
        default:
            fe_db_writef(db, "[TODO]");
            break;
        }
        break;
    case FE__MACH_REG:
        ;
        FeVReg vr = inst->vr_out;
        FeVirtualReg* vreg = fe_vreg(f->vregs, vr);
        u8 class = vreg->class;
        u16 real = vreg->real;
        fe_db_writecstr(db, f->mod->target->reg_name(class, real));
        break;
    case FE__XR_INST_BEGIN ... FE__XR_INST_END:
        target->ir_print_args(db, f, inst);
        break;
    default:
        fe_db_writef(db, "[TODO LMFAO]");
    }
    fe_db_writecstr(db, "\n");
}

void fe_emit_ir_func(FeDataBuffer* db, FeFunc* f, bool fancy) {
    should_ansi = fancy;
    // number all instructions and blocks
    u32 inst_counter = 1;
    u32 block_counter = 1;
    for_blocks(block, f) {
        block->flags = block_counter++;
        for_inst(inst, block) {
            if (inst->ty != FE_TY_VOID)
                inst->flags = inst_counter++;
        }
    }

    switch (f->sym->bind) {
    case FE_BIND_EXTERN: fe_db_writecstr(db, "extern "); break;
    case FE_BIND_GLOBAL: fe_db_writecstr(db, "global "); break;
    case FE_BIND_LOCAL:  fe_db_writecstr(db, "local "); break;
    case FE_BIND_SHARED_EXPORT: fe_db_writecstr(db, "shared_export "); break;
    case FE_BIND_SHARED_IMPORT: fe_db_writecstr(db, "shared_import "); break;
    }

    // write function signature
    fe_db_writef(db, "func \"%.*s\"", f->sym->name.len, fe_compstr_data(f->sym->name));
    if (f->sig->param_len) {
        fe_db_writecstr(db, " ");
        for_n(i, 0, f->sig->param_len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            // fe__emit_ir_ref(db, f, fe_func_param(f, i));
            // fe_db_writecstr(db, ": ");
            fe_db_writecstr(db, ty_name[fe_funcsig_param(f->sig, i)->ty]);
        }
    }
    if (f->sig->return_len) {
        fe_db_writecstr(db, " -> ");
        for_n(i, 0, f->sig->return_len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe_db_writecstr(db, ty_name[fe_funcsig_return(f->sig, i)->ty]);
        }
    }

    if (f->sym->bind == FE_BIND_EXTERN) {
        return;
    }

    // write function body
    fe_db_writecstr(db, " {\n");
    
    // write stack frame
    u32 stack_counter = 1;
    for (FeStackItem* item = f->stack_top; item != nullptr; item = item->next) {
        item->flags = stack_counter;
        fe_db_writef(db, "    s%d: ", stack_counter);
        print_ty(db, item->ty, item->complex_ty);
        fe_db_writecstr(db, "\n");
        stack_counter++;
    }

    for_blocks(block, f) {
        fe_db_writecstr(db, "  ");
        fe__emit_ir_block_label(db, f, block);
        fe_db_writecstr(db, "\n");
        
        // print live-in set if present
        if (block->live) {
            fe_db_writecstr(db, "  in ");
            for_n(i, 0, block->live->in_len) {
                FeVReg in = block->live->in[i];
                fe_db_writef(db, "vr%u, ", in);
            }
            fe_db_writecstr(db, "\n");
        }

        for_inst(inst, block) {
            fe_db_writecstr(db, "    ");
            print_inst(f, db, inst);
        }
        // print live-in set if present
        if (block->live) {
            fe_db_writecstr(db, "  out ");
            for_n(i, 0, block->live->out_len) {
                FeVReg out = block->live->out[i];
                fe_db_writef(db, "vr%u, ", out);
            }
            fe_db_writecstr(db, "\n");
        }
    }
    fe_db_writecstr(db, "}\n");
}
