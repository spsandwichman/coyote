#include "iron/iron.h"

typedef struct FeRecordField FeRecordField;
typedef union FeRecord FeRecord;

typedef struct FeRecordField {
    FeTy ty;
    u16 offset;
    FeRecord* record_ty; // if applicable
} FeRecordField;

typedef union FeRecord {
    FeTy ty;
    struct {
        FeTy ty; // appearing here too for compact layout
        
        u32 len;
        FeRecordField fields[];
    } record;
    struct {
        FeTy ty;
        FeTy elem_ty;

        u32 len;
        FeRecord* elem_record_ty; // if applicable
    } array;
} FeRecord;
