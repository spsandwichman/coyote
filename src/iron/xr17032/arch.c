#include "iron/iron.h"
#include "xr.h"

FeRegClass fe_xr_choose_regclass(FeInstKind kind, FeTy ty) {
    switch (ty) {
    case FE_TY_I8:
    case FE_TY_I16:
    case FE_TY_I32:
    case FE_TY_I64:
    case FE_TY_BOOL:
        return XR_REGCLASS_GPR;
    default:
        FE_CRASH("cant select regclass for type %s", fe_ty_name(ty));
    }
}

void fe_xr_print_inst(FeDataBuffer* db, FeFunc* f, FeInst* inst) {
    const char* name = nullptr;
    switch (inst->kind) {
    case XR_J:   name = "xr.j"; break;
    case XR_JAL: name = "xr.jal"; break;

    case XR_BEQ: name = "xr.beq"; break;
    case XR_BNE: name = "xr.bne"; break;
    case XR_BLT: name = "xr.blt"; break;
    case XR_BLE: name = "xr.ble"; break;
    case XR_BGT: name = "xr.bgt"; break;
    case XR_BGE: name = "xr.bge"; break;
    case XR_BPE: name = "xr.bpe"; break;
    case XR_BPO: name = "xr.bpo"; break;

    case XR_ADDI:   name = "xr.addi"; break;
    case XR_SUBI:   name = "xr.subi"; break;
    case XR_SLTI:   name = "xr.slti"; break;
    case XR_SLTI_S: name = "xr.slti-s"; break;
    case XR_ANDI:   name = "xr.andi"; break;
    case XR_ORI:    name = "xr.ori"; break;
    case XR_LUI:    name = "xr.lui"; break;

    case XR_LOAD8_IO:   name = "xr.load8-io"; break;
    case XR_LOAD16_IO:  name = "xr.load16-io"; break;
    case XR_LOAD32_IO:  name = "xr.load32-io"; break;
    case XR_STORE8_IO:  name = "xr.store8-io"; break;
    case XR_STORE16_IO: name = "xr.store16-io"; break;
    case XR_STORE32_IO: name = "xr.store32-io"; break;

    case XR_LOAD8_RO:   name = "xr.load8-ro"; break;
    case XR_LOAD16_RO:  name = "xr.load16-ro"; break;
    case XR_LOAD32_RO:  name = "xr.load32-ro"; break;
    case XR_STORE8_RO:  name = "xr.store8-ro"; break;
    case XR_STORE16_RO: name = "xr.store16-ro"; break;
    case XR_STORE32_RO: name = "xr.store32-ro"; break;
    
    case XR_STORE8_SI:  name = "xr.store8-si"; break;
    case XR_STORE16_SI: name = "xr.store16-si"; break;
    case XR_STORE32_SI: name = "xr.store32-si"; break;

    case XR_LSH:   name = "xr.lsh"; break;
    case XR_RSH:   name = "xr.rsh"; break;
    case XR_ASH:   name = "xr.ash"; break;
    case XR_ADD:   name = "xr.add"; break;
    case XR_SUB:   name = "xr.sub"; break;
    case XR_SLT:   name = "xr.slt"; break;
    case XR_SLT_S: name = "xr.slt-s"; break;
    case XR_AND:   name = "xr.and"; break;
    case XR_XOR:   name = "xr.xor"; break;
    case XR_OR:    name = "xr.or"; break;
    case XR_NOR:   name = "xr.nor"; break;
    
    case XR_MUL:   name = "xr.mul"; break;
    case XR_DIV:   name = "xr.div"; break;
    case XR_DIV_S: name = "xr.div-s"; break;
    case XR_MOD:   name = "xr.mod"; break;
    
    case XR_LL:  name = "xr.ll"; break;
    case XR_SC:  name = "xr.sc"; break;
    case XR_MB:  name = "xr.mb"; break;
    case XR_WMB: name = "xr.wmb"; break;
    case XR_BRK: name = "xr.brk"; break;
    case XR_SYS: name = "xr.sys"; break;
    
    case XR_MFCR: name = "xr.mfcr"; break;
    case XR_MTCR: name = "xr.mtcr"; break;
    case XR_HLT:  name = "xr.hlt"; break;
    case XR_RFE:  name = "xr.rfe"; break;
    }

    fe_db_writecstr(db, name);
    fe_db_writecstr(db, " ");
}