#include "iron/iron.h"
#include <stdio.h>

static void quick_print(FeFunc* f) {
    FeDataBuffer db;
    fe_db_init(&db, 512);
    fe_emit_ir_func(&db, f, true);
    printf("%.*s", (int)db.len, db.at);
}

FeInst* fe_solve_alias_space(FeFunc* f, u32 alias_space);

FeFunc* make_store_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeSection* text = fe_section_new(mod, "text", 0, FE_SECTION_EXECUTABLE);

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_C, 2, 1);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_I64;
    fe_funcsig_param(f_sig, 1)->ty = FE_TY_I64;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I64;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "foo", 0, text, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;

    FeInst* store = fe_inst_store(
        f, 
        fe_func_param(f, 0), 
        fe_func_param(f, 1), 
        FE_MEMOP_ALIGN_DEFAULT, 0
    );
    fe_append_end(entry, store);

    FeInst* load = fe_inst_load(
        f, 
        FE_TY_I64, 
        fe_func_param(f, 0), 
        FE_MEMOP_ALIGN_DEFAULT, 0
    );
    fe_append_end(entry, load);

    FeInst* zero = fe_inst_const(f, FE_TY_I64, 0);
    fe_append_end(entry, zero);

    FeInst* zero_store = fe_inst_store(
        f, 
        fe_func_param(f, 0), 
        zero, 
        FE_MEMOP_ALIGN_DEFAULT, 0
    );
    fe_append_end(entry, zero_store);

    FeInst* ret = fe_inst_return(f);
    fe_append_end(entry, ret);
    fe_return_set_arg(f, ret, 0, load);
    
    fe_solve_alias_space(f, 0);

    return f;
}

FeFunc* make_alg_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeSection* text = fe_section_new(mod, "text", 0, FE_SECTION_EXECUTABLE);

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_C, 1, 1);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_I64;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I64;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "foo", 0, text, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;

    FeInst* x = fe_inst_const(f, FE_TY_I64, 0x20);
    fe_append_end(entry, x);

    FeInst* add_x = fe_inst_binop(f, 
        FE_TY_I64, FE_IADD, 
        fe_func_param(f, 0),
        x
    );
    fe_append_end(entry, add_x);

    FeInst* y = fe_inst_const(f, FE_TY_I64, 0x10);
    fe_append_end(entry, y);
    
    FeInst* sub_y = fe_inst_binop(f, 
        FE_TY_I64, FE_ISUB,
        add_x,
        y
    );
    fe_append_end(entry, sub_y);

    FeInst* ret = fe_inst_return(f);
    fe_append_end(entry, ret);
    fe_return_set_arg(f, ret, 0, sub_y);
    
    fe_solve_alias_space(f, 0);

    return f;
}

int main() {
    fe_init_signal_handler();
    FeInstPool ipool;
    fe_ipool_init(&ipool);
    FeVRegBuffer vregs;
    fe_vrbuf_init(&vregs, 2048);

    FeModule* mod = fe_module_new(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    FeFunc* func = make_alg_test(mod, &ipool, &vregs);

    quick_print(func);
    fe_opt_local(func);
    quick_print(func);

    // fe_opt_algsimp(func);
    // quick_print(func);
    // fe_codegen(func);
    // quick_print(func);

    // printf("------ final assembly ------\n");

    // FeDataBuffer db; 
    // fe_db_init(&db, 2048);
    // fe_emit_asm(&db, mod);
    // printf("%.*s", (int)db.len, db.at);

    // fe_module_destroy(mod);
}
