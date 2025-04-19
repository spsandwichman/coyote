#define ORBIT_IMPLEMENTATION
#include <stdio.h>
#include "orbit.h"

#include "lex.h"

#include "iron/iron.h"

static void quick_print(FeFunction* f) {
    FeDataBuffer db;
    fe_db_init(&db, 512);
    fe_print_func(&db, f);
    printf("%.*s", db.len, db.at);
}


FeFunction* make_factorial(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    // set up function to call
    FeFuncSignature* fact_sig = fe_new_funcsig(FE_CC_JACKAL, 1, 1);
    fe_funcsig_param(fact_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(fact_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* fact_sym = fe_new_symbol(mod, "factorial", 0, FE_BIND_GLOBAL);
    FeFunction* fact = fe_new_function(mod, fact_sym, fact_sig, ipool, vregs);

    // construct the function's body
    FeBlock* entry = fact->entry_block;
    FeBlock* if_true = fe_new_block(fact);
    FeBlock* if_false = fe_new_block(fact);
    FeInst* param = fe_func_param(fact, 0);

    { // entry block
        FeInst* const0 = fe_append_end(entry, fe_inst_const(fact, FE_TY_I32, 0));
        FeInst* eq = fe_append_end(entry, fe_inst_binop(fact, 
            FE_TY_I32, FE_EQ,
            param,
            const0
        ));
        fe_append_end(entry, fe_inst_branch(fact, eq, if_true, if_false));
    }
    { // if_true block
        FeInst* const1 = fe_append_end(if_true, fe_inst_const(fact, FE_TY_I32, 1));
        FeInst* ret = fe_append_end(if_true, fe_inst_return(fact));
        fe_set_return_arg(ret, 0, const1);
    }
    { // if_false block
        FeInst* const1 = fe_append_end(if_false, fe_inst_const(fact, FE_TY_I32, 1));
        FeInst* isub = fe_append_end(if_false, fe_inst_binop(fact, 
            FE_TY_I32, FE_ISUB,
            param,
            const1
        ));
        FeInst* call = fe_append_end(if_false, fe_inst_call_direct(fact, fact));
        fe_set_call_arg(call, 0, isub);
        FeInst* imul = fe_append_end(if_false, fe_inst_binop(fact, 
            FE_TY_I32, FE_IMUL,
            param,
            call
        ));

        FeInst* ret = fe_append_end(if_false, fe_inst_return(fact));
        fe_set_return_arg(ret, 0, imul);
    }
    return fact;
}


FeFunction* make_branch_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    // set up function to call
    FeFuncSignature* fact_sig = fe_new_funcsig(FE_CC_JACKAL, 1, 1);
    fe_funcsig_param(fact_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(fact_sig, 0)->ty = FE_TY_I32;

    // make the function and its symbol
    FeSymbol* fact_sym = fe_new_symbol(mod, "branch_test", 0, FE_BIND_GLOBAL);
    FeFunction* fact = fe_new_function(mod, fact_sym, fact_sig, ipool, vregs);

    // construct the function's body
    FeBlock* entry = fact->entry_block;
    FeBlock* if_true = fe_new_block(fact);
    FeBlock* if_false = fe_new_block(fact);
    FeInst* param = fe_func_param(fact, 0);

    { // entry block
        FeInst* const0 = fe_append_end(entry, fe_inst_const(fact, FE_TY_I32, 0));
        FeInst* eq = fe_append_end(entry, fe_inst_binop(fact, 
            FE_TY_I32, FE_EQ,
            param,
            const0
        ));
        fe_append_end(entry, fe_inst_branch(fact, eq, if_true, if_false));
    }
    { // if_true block
        FeInst* const1 = fe_append_end(if_true, fe_inst_const(fact, FE_TY_I32, 0xFFFF0000));
        FeInst* ret = fe_append_end(if_true, fe_inst_return(fact));
        fe_set_return_arg(ret, 0, const1);
    }
    { // if_false block
        FeInst* const1 = fe_append_end(if_false, fe_inst_const(fact, FE_TY_I32, 0xFFFF));
        FeInst* ret = fe_append_end(if_false, fe_inst_return(fact));
        fe_set_return_arg(ret, 0, const1);
    }
    return fact;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("no file provided\n");
        return 1;
    }

    char* filepath = argv[1];

    FsFile* file = fs_open(filepath, false, false);
    if (file == NULL) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    SrcFile f = {
        .src = fs_read_entire(file),
        .path = fs_from_path(&file->path),
    };

    Vec(Token) tokens = lex_entrypoint(&f);
    (void)tokens;
    // printf("%zu chars -> %zu tokens (%zuB used, %zuB capacity)\n", file->size, tokens.len, sizeof(Token)*tokens.len, sizeof(Token)*tokens.cap);

    // for_vec(Token* t, &tokens) {
    //     if (_TOK_PREPROC_BEGIN < t->kind && t->kind < _TOK_PREPROC_END) {
    //         printf("%s ", token_kind[t->kind]);
    //         continue;
    //     }
    //     if (t->kind == TOK_STRING) {
    //         printf("\"");
    //     }
    //     printf(str_fmt, str_arg(tok_span(*t)));
    //     if (t->kind == TOK_STRING) {
    //         printf("\"");
    //     }
    //     printf(" ");
    // }
    // printf("\n");

    fe_init_signal_handler();
    FeInstPool ipool;
    fe_ipool_init(&ipool);
    FeVRegBuffer vregs;
    fe_vrbuf_init(&vregs, 2048);

    FeModule* mod = fe_new_module(FE_ARCH_XR17032);

    FeFunction* func = make_branch_test(mod, &ipool, &vregs);
    
    quick_print(func);
    // fe_calculate_cfg(func);
    fe_isel(func);
    quick_print(func);
}