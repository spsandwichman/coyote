#include "iron/iron.h"

usize fe_xr_extra_size_unsafe(FeInstKind kind) {
    switch (kind) {
    case FE_XR_ADDI:
    case FE_XR_SUBI:
        return sizeof(FeXrRegImm16);
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        return sizeof(FeXrRegReg);
    case FE_XR_RET:
        return 0;
    default:
        return 255;
    }
}

char* fe_xr_inst_name(FeInstKind kind) {
    switch (kind) {
    case FE_XR_ADDI: return "xr.addi";
    case FE_XR_SUBI: return "xr.subi";
    case FE_XR_ADD:  return "xr.add";
    case FE_XR_SUB:  return "xr.sub";
    case FE_XR_MUL:  return "xr.mul";

    case FE_XR_RET:  return "xr.ret";
    }
    return "xr.???";
}

void fe_xr_print_args(FeDataBuffer* db, FeInst* inst) {
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_SUBI:
        fe__print_ref(db, fe_extra_T(inst, FeXrRegImm16)->val);
        fe_db_writef(db, ", %u", fe_extra_T(inst, FeXrRegImm16)->num);
        break;    
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        fe__print_ref(db, fe_extra_T(inst, FeXrRegReg)->lhs);
        fe_db_writecstr(db, ", ");
        fe__print_ref(db, fe_extra_T(inst, FeXrRegReg)->rhs);
        break;    
    case FE_XR_RET:
        break;
    default:
        fe_db_writecstr(db, "???");
    }
}

FeInst** fe_xr_list_inputs(FeInst* inst, usize* len_out) {
    switch (inst->kind) {
    case FE_XR_ADDI:
    case FE_XR_SUBI:
        *len_out = 1;
        return &fe_extra_T(inst, FeXrRegImm16)->val;
    case FE_XR_ADD:
    case FE_XR_SUB:
    case FE_XR_MUL:
        *len_out = 2;
        return &fe_extra_T(inst, FeXrRegReg)->lhs;
    case FE_XR_RET:
        *len_out = 0;
        return NULL;
    default:
        fe_runtime_crash("list_inputs: unknown kind %d", inst->kind);
        break;
    }
}