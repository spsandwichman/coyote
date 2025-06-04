#include "iron/iron.h"
#include <stdio.h>

static void quick_print(FeFunc* f) {
    FeDataBuffer db;
    fe_db_init(&db, 512);
    fe_emit_ir_func(&db, f, true);
    printf("%.*s", (int)db.len, db.at);
}


FeFunc* make_phi_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 1, 1);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_BOOL;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

    FeSymbol* f_sym = fe_symbol_new(mod, "phi_test", 0, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;
    FeBlock* if_true = fe_block_new(f);
    FeBlock* if_false = fe_block_new(f);
    FeBlock* phi_block = fe_block_new(f);

    {
        FeInst* branch = fe_append_end(entry, fe_inst_branch(f, f->params[0], if_true, if_false));
    }
    FeInst* if_true_const;
    {
        if_true_const = fe_append_end(if_true, fe_inst_const(f, FE_TY_I32, 10));
        FeInst* jump = fe_append_end(if_true, fe_inst_jump(f, phi_block));
    }
    FeInst* if_false_const;
    {
        if_false_const = fe_append_end(if_false, fe_inst_const(f, FE_TY_I32, 3));
        FeInst* jump = fe_append_end(if_false, fe_inst_jump(f, phi_block));
    }
    {
        FeInst* phi = fe_append_end(phi_block, fe_inst_phi(f, FE_TY_I32, 2));
        fe_phi_set_src(phi, 0, if_true_const, if_true);
        fe_phi_set_src(phi, 1, if_false_const, if_false);
        FeInst* ret = fe_append_end(phi_block, fe_inst_return(f));
        fe_return_set_arg(ret, 0, phi);
    }

    return f;
}

FeFunc* make_factorial(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    // set up function to call
    FeFuncSig* fact_sig = fe_funcsig_new(FE_CCONV_JACKAL, 1, 1);
    fe_funcsig_param(fact_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(fact_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* fact_sym = fe_symbol_new(mod, "factorial", 0, FE_BIND_GLOBAL);
    FeFunc* fact = fe_func_new(mod, fact_sym, fact_sig, ipool, vregs);

    // construct the function's body
    FeBlock* entry = fact->entry_block;
    FeBlock* if_true = fe_block_new(fact);
    FeBlock* if_false = fe_block_new(fact);
    FeInst* param = fe_func_param(fact, 0);

    { // entry block
        FeInst* const0 = fe_append_end(entry, fe_inst_const(fact, FE_TY_I32, 0));
        FeInst* eq = fe_append_end(entry, fe_inst_binop(fact, 
            FE_TY_I32, FE_IEQ,
            param,
            const0
        ));
        fe_append_end(entry, fe_inst_branch(fact, eq, if_true, if_false));
    }
    { // if_true block
        FeInst* const1 = fe_append_end(if_true, fe_inst_const(fact, FE_TY_I32, 1));
        FeInst* ret = fe_append_end(if_true, fe_inst_return(fact));
        fe_return_set_arg(ret, 0, const1);
    }
    { // if_false block
        FeInst* const1 = fe_append_end(if_false, fe_inst_const(fact, FE_TY_I32, 1));
        FeInst* isub = fe_append_end(if_false, fe_inst_binop(fact, 
            FE_TY_I32, FE_ISUB,
            param,
            const1
        ));
        FeInst* fact_symaddr = fe_append_end(if_false, fe_inst_sym_addr(fact, FE_TY_I32, fact->sym));
        FeInst* call = fe_append_end(if_false, fe_inst_call(fact, fact_symaddr, fact->sig));
        fe_call_set_arg(call, 0, isub);
        FeInst* imul = fe_append_end(if_false, fe_inst_binop(fact, 
            FE_TY_I32, FE_IMUL,
            param,
            call
        ));

        FeInst* ret = fe_append_end(if_false, fe_inst_return(fact));
        fe_return_set_arg(ret, 0, imul);
    }
    return fact;
}

FeFunc* make_factorial2(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    // set up function to call
    FeFuncSig* fact_sig = fe_funcsig_new(FE_CCONV_JACKAL, 1, 1);
    fe_funcsig_param(fact_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(fact_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* fact_sym = fe_symbol_new(mod, "factorial", 0, FE_BIND_GLOBAL);
    FeFunc* fact = fe_func_new(mod, fact_sym, fact_sig, ipool, vregs);

    // construct the function's body
    FeBlock* entry = fact->entry_block;
    FeBlock* if_true = fe_block_new(fact);
    FeBlock* if_false = fe_block_new(fact);
    FeInst* param = fe_func_param(fact, 0);

    { // entry block
        FeInst* const0 = fe_append_end(entry, fe_inst_const(fact, FE_TY_I32, 0));
        FeInst* eq = fe_append_end(entry, fe_inst_binop(fact,
            FE_TY_I32, FE_IEQ,
            param,
            const0
        ));
        fe_append_end(entry, fe_inst_branch(fact, eq, if_true, if_false));
        // fe_append_end(entry, fe_inst_jump(fact, if_false));
    }
    { // if_true block
        FeInst* const1 = fe_append_end(if_true, fe_inst_const(fact, FE_TY_I32, 1));
        FeInst* ret = fe_append_end(if_true, fe_inst_return(fact));
        fe_return_set_arg(ret, 0, const1);
    }
    {  // if_false block
        FeInst* const1 = fe_append_end(if_false, fe_inst_const(fact, FE_TY_I32, 1));
        FeInst* isub = fe_append_end(if_false, fe_inst_binop(fact,
            FE_TY_I32, FE_ISUB,
            param,
            const1
        ));
        // FeInst* fact_symaddr = fe_append_end(if_false, fe_inst_sym_addr(fact, FE_TY_I32, fact->sym));
        // FeInst* call = fe_append_end(if_false, fe_inst_call(fact, fact_symaddr, fact->sig));
        // FeInst* call = fe_append_end(if_false, fe_inst_unop(fact, FE_TY_I32, FE_MOV, isub));
        // fe_call_set_arg(call, 0, isub);
        FeInst* imul = fe_append_end(if_false, fe_inst_binop(fact,
            FE_TY_I32, FE_IMUL,
            param,
            isub
        ));

        FeInst* ret = fe_append_end(if_false, fe_inst_return(fact));
        fe_return_set_arg(ret, 0, imul);
    }
    return fact;
}

FeFunc* make_branch_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    // set up function to call
    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 1, 1);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "branch_test", 0, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    // construct the function's body
    FeBlock* entry = f->entry_block;
    FeBlock* if_true = fe_block_new(f);
    FeBlock* if_false = fe_block_new(f);
    FeInst* param = fe_func_param(f, 0);

    { // entry block
        FeInst* const0 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 0));
        FeInst* eq = fe_append_end(entry, fe_inst_binop(f, 
            FE_TY_I32, FE_IEQ,
            param,
            const0
        ));
        fe_append_end(entry, fe_inst_branch(f, eq, if_true, if_false));
    }
    { // if_true block
        FeInst* const1 = fe_append_end(if_true, fe_inst_const(f, FE_TY_I32, 0xAFFF0000));
        FeInst* ret = fe_append_end(if_true, fe_inst_return(f));
        fe_return_set_arg(ret, 0, const1);
    }
    { // if_false block
        FeInst* add = fe_append_end(if_false, fe_inst_binop(f, 
            FE_TY_I32, FE_IADD, 
            param,
            fe_append_end(if_false, fe_inst_const(f, FE_TY_I32, 10))
        ));
        FeInst* ret = fe_append_end(if_false, fe_inst_return(f));
        fe_return_set_arg(ret, 0, add);
    }
    return f;
}

FeFunc* make_regalloc_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    // set up function to call
    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 4, 4);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_param(f_sig, 1)->ty = FE_TY_I32;
    fe_funcsig_param(f_sig, 2)->ty = FE_TY_I32;
    fe_funcsig_param(f_sig, 3)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 1)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 2)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 3)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "id", 0, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    // construct the function's body
    FeBlock* entry = f->entry_block;
    FeInst* param0 = f->params[0];
    FeInst* param1 = fe_func_param(f, 1);
    FeInst* param2 = fe_func_param(f, 2);
    FeInst* param3 = fe_func_param(f, 3);

    { // entry block
        FeInst* ret = fe_append_end(entry, fe_inst_return(f));
        fe_return_set_arg(ret, 0, param0);
        fe_return_set_arg(ret, 1, param1);
        fe_return_set_arg(ret, 2, param2);
        fe_return_set_arg(ret, 3, param3);
    }
    return f;
}

FeFunc* make_symaddr_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 0, 1);
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "symaddr_test", 0, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);
    FeBlock* entry = f->entry_block;

    FeInst* symaddr = fe_append_end(entry, fe_inst_sym_addr(f, FE_TY_I32, f_sym));
    FeInst* ret = fe_append_end(entry, fe_inst_return(f));
    fe_return_set_arg(ret, 0, symaddr);
    
    return f;
}

FeFunc* make_algsimp_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 0, 1);
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "algsimp_test", 0, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);
    FeBlock* entry = f->entry_block;

    FeInst* const1 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 10));
    FeInst* const2 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 20));
    FeInst* const3 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 30));
    FeInst* add = fe_append_end(entry, fe_inst_binop(f, FE_TY_I32, 
        FE_IADD,
        const1, const2
    ));
    FeInst* mul = fe_append_end(entry, fe_inst_binop(f, FE_TY_I32, 
        FE_IDIV,
        add, const3
    ));
    FeInst* ret = fe_append_end(entry, fe_inst_return(f));
    fe_return_set_arg(ret, 0, mul);
    
    return f;
}


FeFunc* make_mem_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 1, 1);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "mem_test", 0, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);
    FeBlock* entry = f->entry_block;

    FeInst* const1 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 10));
    FeInst* const2 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 20));
    FeInst* const3 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 30));
    FeInst* add = fe_append_end(entry, fe_inst_binop(f, FE_TY_I32, 
        FE_IADD,
        const1, const2
    ));
    FeInst* mul = fe_append_end(entry, fe_inst_binop(f, FE_TY_I32, 
        FE_IDIV,
        add, const3
    ));
    FeInst* ret = fe_append_end(entry, fe_inst_return(f));
    fe_return_set_arg(ret, 0, mul);
    
    return f;
}

int main() {
    fe_init_signal_handler();
    FeInstPool ipool;
    fe_ipool_init(&ipool);
    FeVRegBuffer vregs;
    fe_vrbuf_init(&vregs, 2048);

    FeModule* mod = fe_module_new(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    FeFunc* func = make_algsimp_test(mod, &ipool, &vregs);

    quick_print(func);
    fe_opt_algsimp(func);
    quick_print(func);
    fe_codegen(func);
    quick_print(func);

    printf("------ final assembly ------\n");

    FeDataBuffer db; 
    fe_db_init(&db, 2048);
    fe_emit_asm(&db, mod);
    printf("%.*s", (int)db.len, db.at);

    fe_module_destroy(mod);
}
