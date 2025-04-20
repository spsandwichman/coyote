#include <iron/iron.h>

// algsimp - algebraic simplification

/*
    constant evaluation:
        1 + 2   -> 3
        1 - 2   -> -1
        2 * 2   -> 4
        6 / 2   -> 3
        (neg) 1 -> -1
        ...

    strength reduction:
        x * 2 -> x << 1 // extend to powers of two
        x / 2 -> x >> 1 // extend to powers of two
        x % 2 -> x & 1  // extend to powers of two

    identity reduction:
        x + 0      -> x
        x - 0      -> x
        0 - x      -> -x
        x * 0      -> 0
        x * 1      -> x
        x / 1      -> x
        0 / x      -> 0 -- watch out for x == 0 ???
        x & 0      -> 0
        x | 0      -> x
        x ^ 0      -> x
        x << 0     -> x
        x >> 0     -> x
        ~(~x)      -> x
        -(-x)      -> x
        0 << x     -> 0
        0 >> x     -> 0

        (these only applies to bool):
            x & false -> false
            x & true  -> x
            x | false -> x
            x | true  -> true

    reassociation:
        (x + 1) + 2  ->  x + (1 + 2)
        (x * 1) * 2  ->  x * (1 * 2)
        (x & 1) & 2  ->  x & (1 & 2)
        (x | 1) | 2  ->  x | (1 | 2)
        (x ^ 1) ^ 2  ->  x ^ (1 ^ 2)
*/

static bool is_zero(FeInst* inst) {
    return inst->kind == FE_CONST && fe_extra_T(inst, FeInstConst)->val == 0;
}

static FeInst* identity(FeInst* inst) {
    FeInstBinop* inst_bin = fe_extra(inst);

    switch (inst->kind) {
    case FE_IADD:
        if (is_zero(inst_bin->rhs)) {
            return inst_bin->lhs;
        }
        break;
    }

    return inst;
}

void fe_opt_algsimp(FeFunction* f) {
    FeTarget* t = f->mod->target;

    for_blocks(b, f) {
        for_inst(inst, b) {
            usize len;
            FeInst** inputs = fe_inst_list_inputs(t, inst, &len);
            for_n (i, 0, len) {
                inputs[i] = identity(inputs[i]);
            }
        }
    }
}