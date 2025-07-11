#include "iron/iron.h"
#include <stdio.h>

static void quick_print(FeFunc* f) {
    FeDataBuffer db;
    fe_db_init(&db, 512);
    fe_emit_ir_func(&db, f, true);
    printf("%.*s", (int)db.len, db.at);
}



int main() {
    fe_init_signal_handler();
    FeInstPool ipool;
    fe_ipool_init(&ipool);
    FeVRegBuffer vregs;
    fe_vrbuf_init(&vregs, 2048);

    FeModule* mod = fe_module_new(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    // FeFunc* func = make_stack_test(mod, &ipool, &vregs);

    // quick_print(func);
    // fe_opt_tdce(func);
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
