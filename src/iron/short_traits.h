#ifndef IRON_SHORT_TRAITS_H
#define IRON_SHORT_TRAITS_H

#include "iron.h"
enum {
    // if something is commutative, it is also fast-commutative.
    COMMU       = FE_TRAIT_COMMUTATIVE | FE_TRAIT_FAST_COMMUTATIVE,
    FAST_COMMU  = FE_TRAIT_FAST_COMMUTATIVE,
    // same here.
    ASSOC       = FE_TRAIT_ASSOCIATIVE | FE_TRAIT_FAST_ASSOCIATIVE,
    FAST_ASSOC  = FE_TRAIT_FAST_ASSOCIATIVE,

    VOL         = FE_TRAIT_VOLATILE,
    // term always implies volatile
    TERM        = FE_TRAIT_TERMINATOR | FE_TRAIT_VOLATILE,
    SAME_IN_OUT = FE_TRAIT_SAME_IN_OUT_TY,
    SAME_INS    = FE_TRAIT_SAME_INPUT_TYS,
    INT_IN      = FE_TRAIT_INT_INPUT_TYS,
    FLT_IN      = FE_TRAIT_FLT_INPUT_TYS,
    VEC_IN      = FE_TRAIT_VEC_INPUT_TYS,
    BOOL_OUT    = FE_TRAIT_BOOL_OUT_TY,

    MOV_HINT    = FE_TRAIT_REG_MOV_HINT,
};

#endif // IRON_SHORT_TRAITS_H