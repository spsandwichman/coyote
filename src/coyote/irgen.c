#include "common/util.h"

#include "parse.h"
#include "iron/iron.h"

typedef struct {
    FeFunc* func;
    FeBlock* block;

    FeTy uword;
    FeTy word;
} IrGen;


static FeInstKind expr_kind_to_inst_kind(ExprKind kind, bool is_signed) {
    switch (kind) {
    case EXPR_ADD: return FE_IADD;
    case EXPR_SUB: return FE_ISUB;
    case EXPR_MUL: return FE_IMUL;
    case EXPR_DIV: return is_signed ? FE_IDIV : FE_UDIV;
    case EXPR_REM: return is_signed ? FE_IDIV : FE_UDIV;
    case EXPR_AND: return FE_AND;
    case EXPR_OR:  return FE_OR;
    case EXPR_XOR: return FE_XOR;
    case EXPR_LSH: return FE_SHL;
    case EXPR_RSH: return is_signed ? FE_ISR : FE_USR;
    default:
        UNREACHABLE;
    }
}

FeInst* irgen_expr(IrGen* gen, Expr* expr) {
    switch (expr->kind) {
    case EXPR_ADD ... EXPR_RSH: {
        FeInst* left = irgen_expr(gen, expr->binary.lhs);
        FeInst* right = irgen_expr(gen, expr->binary.lhs);
        FeInstKind kind = expr_kind_to_inst_kind(kind, false); // ASSUME NOT SIGNED BAD LMAO
        FeInst* op = fe_inst_binop(gen->func, left->ty, kind, left, right);
        return op;
    } break;
    case EXPR_LITERAL: {
        FeInst* lit = fe_inst_const(gen->func, gen->uword, expr->literal);
    } break;
    default:
        UNREACHABLE;
    }
}
