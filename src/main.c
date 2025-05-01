#include <stdio.h>

#define ORBIT_IMPLEMENTATION
#include "orbit.h"

#include "lex.h"
#include "parse.h"
// #include "iron/iron.h"

// static void quick_print(FeFunction* f) {
//     FeDataBuffer db;
//     fe_db_init(&db, 512);
//     fe_print_func(&db, f);
//     printf("%.*s", (int)db.len, db.at);
// }

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("no file provided\n");
        return 1;
    }

    char* filepath = argv[1];

    FsFile* file = fs_open(filepath, false, false);
    if (file == nullptr) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    SrcFile f = {
        .src = fs_read_entire(file),
        .path = fs_from_path(&file->path),
    };

    Context ctx = lex_entrypoint(&f);
    for_n(i, 0, ctx.tokens_len) {
        Token* t = &ctx.tokens[i];
        if (_TOK_LEX_IGNORE < t->kind && t->kind < _TOK_PREPROC_TRANSPARENT_END) {
            // printf("%s ", token_kind[t->kind]);
            continue;
        }
        // printf(str_fmt, str_arg(tok_span(*t)));
        token_error(&ctx, i, i + 3, "error on 'x'");
        break;
        // printf(" ");
    }

    // fe_init_signal_handler();
    // FeInstPool ipool;
    // fe_ipool_init(&ipool);
    // FeVRegBuffer vregs;
    // fe_vrbuf_init(&vregs, 2048);

    // FeModule* mod = fe_new_module(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    // FeFunction* func = make_factorial(mod, &ipool, &vregs);
    
    // quick_print(func);
    // fe_codegen(func);
    // quick_print(func);

    // printf("------ final assembly ------\n");

    // FeDataBuffer db; 
    // fe_db_init(&db, 2048);
    // fe_emit_asm(func, &db);
    // printf("%.*s", (int)db.len, db.at);
}

// FeFunction* make_factorial(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

//     // set up function to call
//     FeFuncSignature* fact_sig = fe_new_funcsig(FE_CCONV_JACKAL, 1, 1);
//     fe_funcsig_param(fact_sig, 0)->ty = FE_TY_I32;
//     fe_funcsig_return(fact_sig, 0)->ty = FE_TY_I32;

//     // make the function and its symbol
//     FeSymbol* fact_sym = fe_new_symbol(mod, "factorial", 0, FE_BIND_GLOBAL);
//     FeFunction* fact = fe_new_function(mod, fact_sym, fact_sig, ipool, vregs);

//     // construct the function's body
//     FeBlock* entry = fact->entry_block;
//     FeBlock* if_true = fe_new_block(fact);
//     FeBlock* if_false = fe_new_block(fact);
//     FeInst* param = fe_func_param(fact, 0);

//     { // entry block
//         FeInst* const0 = fe_append_end(entry, fe_inst_const(fact, FE_TY_I32, 0));
//         FeInst* eq = fe_append_end(entry, fe_inst_binop(fact, 
//             FE_TY_I32, FE_EQ,
//             param,
//             const0
//         ));
//         fe_append_end(entry, fe_inst_branch(fact, eq, if_true, if_false));
//     }
//     { // if_true block
//         FeInst* const1 = fe_append_end(if_true, fe_inst_const(fact, FE_TY_I32, 1));
//         FeInst* ret = fe_append_end(if_true, fe_inst_return(fact));
//         fe_set_return_arg(ret, 0, const1);
//     }
//     { // if_false block
//         FeInst* const1 = fe_append_end(if_false, fe_inst_const(fact, FE_TY_I32, 1));
//         FeInst* isub = fe_append_end(if_false, fe_inst_binop(fact, 
//             FE_TY_I32, FE_ISUB,
//             param,
//             const1
//         ));
//         FeInst* call = fe_append_end(if_false, fe_inst_call_direct(fact, fact));
//         fe_set_call_arg(call, 0, isub);
//         FeInst* imul = fe_append_end(if_false, fe_inst_binop(fact, 
//             FE_TY_I32, FE_IMUL,
//             param,
//             call
//         ));

//         FeInst* ret = fe_append_end(if_false, fe_inst_return(fact));
//         fe_set_return_arg(ret, 0, imul);
//     }
//     return fact;
// }

// FeFunction* make_branch_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

//     // set up function to call
//     FeFuncSignature* f_sig = fe_new_funcsig(FE_CCONV_JACKAL, 1, 1);
//     fe_funcsig_param(f_sig, 0)->ty = FE_TY_I32;
//     fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

//     // make the function and its symbol
//     FeSymbol* f_sym = fe_new_symbol(mod, "branch_test", 0, FE_BIND_GLOBAL);
//     FeFunction* f = fe_new_function(mod, f_sym, f_sig, ipool, vregs);

//     // construct the function's body
//     FeBlock* entry = f->entry_block;
//     FeBlock* if_true = fe_new_block(f);
//     FeBlock* if_false = fe_new_block(f);
//     FeInst* param = fe_func_param(f, 0);

//     { // entry block
//         FeInst* const0 = fe_append_end(entry, fe_inst_const(f, FE_TY_I32, 0));
//         FeInst* eq = fe_append_end(entry, fe_inst_binop(f, 
//             FE_TY_I32, FE_EQ,
//             param,
//             const0
//         ));
//         fe_append_end(entry, fe_inst_branch(f, eq, if_true, if_false));
//     }
//     { // if_true block
//         FeInst* const1 = fe_append_end(if_true, fe_inst_const(f, FE_TY_I32, 0xAFFF0000));
//         FeInst* ret = fe_append_end(if_true, fe_inst_return(f));
//         fe_set_return_arg(ret, 0, const1);
//     }
//     { // if_false block
//         FeInst* add = fe_append_end(if_false, fe_inst_binop(f, 
//             FE_TY_I32, FE_ISUB, 
//             param,
//             fe_append_end(if_false, fe_inst_const(f, FE_TY_I32, 10))
//         ));
//         FeInst* ret = fe_append_end(if_false, fe_inst_return(f));
//         fe_set_return_arg(ret, 0, add);
//     }
//     return f;
// }

// FeFunction* make_regalloc_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

//     // set up function to call
//     FeFuncSignature* f_sig = fe_new_funcsig(FE_CCONV_JACKAL, 4, 4);
//     fe_funcsig_param(f_sig, 0)->ty = FE_TY_I32;
//     fe_funcsig_param(f_sig, 1)->ty = FE_TY_I32;
//     fe_funcsig_param(f_sig, 2)->ty = FE_TY_I32;
//     fe_funcsig_param(f_sig, 3)->ty = FE_TY_I32;
//     fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;
//     fe_funcsig_return(f_sig, 1)->ty = FE_TY_I32;
//     fe_funcsig_return(f_sig, 2)->ty = FE_TY_I32;
//     fe_funcsig_return(f_sig, 3)->ty = FE_TY_I32;

//     // make the function and its symbol
//     FeSymbol* f_sym = fe_new_symbol(mod, "regalloc_test", 0, FE_BIND_GLOBAL);
//     FeFunction* f = fe_new_function(mod, f_sym, f_sig, ipool, vregs);

//     // construct the function's body
//     FeBlock* entry = f->entry_block;
//     FeInst* param0 = fe_func_param(f, 0);
//     FeInst* param1 = fe_func_param(f, 1);
//     FeInst* param2 = fe_func_param(f, 2);
//     FeInst* param3 = fe_func_param(f, 3);

//     { // entry block
//         FeInst* ret = fe_append_end(entry, fe_inst_return(f));
//         fe_set_return_arg(ret, 0, param0);
//         fe_set_return_arg(ret, 1, param1);
//         fe_set_return_arg(ret, 2, param2);
//         fe_set_return_arg(ret, 3, param3);
//     }
//     return f;
// }

// FeFunction* make_phi_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    
//     FeFuncSignature* f_sig = fe_new_funcsig(FE_CCONV_JACKAL, 1, 1);
//     fe_funcsig_param(f_sig, 0)->ty = FE_TY_BOOL;
//     fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

//     FeSymbol* f_sym = fe_new_symbol(mod, "phi_test", 0, FE_BIND_GLOBAL);
//     FeFunction* f = fe_new_function(mod, f_sym, f_sig, ipool, vregs);

//     FeBlock* entry = f->entry_block;
//     FeBlock* if_true = fe_new_block(f);
//     FeBlock* if_false = fe_new_block(f);
//     FeBlock* phi_block = fe_new_block(f);

//     {
//         FeInst* branch = fe_append_end(entry, fe_inst_branch(f, f->params[0], if_true, if_false));
//     }
//     {
//         FeInst* c = fe_append_end(if_true, fe_inst_const(f, FE_TY_I32, 10));
//     }
//     {
//         FeInst* c = fe_append_end(if_false, fe_inst_const(f, FE_TY_I32, 3));
//     }

//     return f;
// }