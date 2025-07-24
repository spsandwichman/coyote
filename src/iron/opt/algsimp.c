#include "iron/iron.h"

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

thread_local static FeWorklist wl = {0};

static bool is_const(FeInst* inst) {
    return inst->kind == FE_CONST;
}

static bool is_val(FeInst* inst, u64 val) {
    return inst->kind == FE_CONST && fe_extra(inst, FeInstConst)->val == val;
}

static u64 val(FeInst* inst) {
    return fe_extra(inst, FeInstConst)->val;
}

static FeInst* consteval(FeFunc* f, FeInst* inst) {
    bool is_binop = fe_inst_has_trait(inst->kind, FE_TRAIT_BINOP);
    // bool is_unop = fe_inst_has_trait(inst->kind, FE_TRAIT_UNOP);

    FeInst* lhs_inst = nullptr;
    FeInst* rhs_inst = nullptr;
    u64 lhs = 0;
    u64 rhs = 0;
    u64 result = 0;
    if (is_binop) {
        // lhs_inst = fe_extra(inst, FeInstBinop)->lhs;
        // rhs_inst = fe_extra(inst, FeInstBinop)->rhs;
        lhs_inst = inst->inputs[0];
        rhs_inst = inst->inputs[1];
        if (!(is_const(lhs_inst) && is_const(rhs_inst))) {
            return inst; // exit early
        }
        lhs = val(lhs_inst);
        rhs = val(rhs_inst);
    }

    switch (inst->kind) {
    case FE_IADD: result = lhs + rhs; break;
    case FE_ISUB: result = lhs - rhs; break;
    case FE_IMUL: result = lhs * rhs; break;
    case FE_IDIV: result = (i64)lhs / (i64)rhs; break;
    case FE_IREM: result = (i64)lhs % (i64)rhs; break;
    case FE_UDIV: result = lhs / rhs; break;
    case FE_UREM: result = lhs % rhs; break;
    case FE_AND:  result = lhs & rhs; break;
    case FE_OR:   result = lhs | rhs; break;
    case FE_XOR:  result = lhs ^ rhs; break;
    case FE_SHL:  result = lhs << rhs; break;
    case FE_USR:  result = lhs >> rhs; break;
    case FE_ISR:  result = (i64)lhs >> (i64)rhs; break;
    case FE_ILT:  result = (bool)((i64)lhs >  (i64)rhs); break;
    case FE_ILE:  result = (bool)((i64)lhs >= (i64)rhs); break;
    case FE_ULT:  result = (bool)(lhs >  rhs); break;
    case FE_ULE:  result = (bool)(lhs >= rhs); break;
    case FE_IEQ:  result = (bool)(lhs == rhs); break;
    default:
        return inst;
    }

    return fe_inst_const(f, inst->ty, result);
}

// static void replace_input(const FeTarget* t, FeInst* user, FeInst* old_def, FeInst* new_def) {
//     for_n(i, 0, len) {
//         if (inputs[i] == old_def) {
//             inputs[i] = new_def;
//         }
//     }
// }

void fe_opt_algsimp(FeFunc* f) {
    FE_ASSERT(false);
}
