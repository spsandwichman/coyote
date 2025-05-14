#include "iron/iron.h"

#define SYMTAB_TOMBSTONE ((void*)0xDEADBEEFBADF00D)

typedef struct Entry {
    FeCompactStr name;
    FeSymbol* sym;
} Entry;

typedef struct FeSymTable {
    Entry* vals;
    u32 cap;
} FeSymTable;

static usize hash(const char* data, u16 len) {

}

void fe_sym_table_init(FeSymTable* st) {
    st->cap = 256;
    st->vals = fe_malloc(sizeof(st->vals[0]) * st->cap);
    memset(st->vals, 0, sizeof(st->vals[0]) * st->cap);
}

void fe_sym_table_put(FeSymTable* st, FeSymbol* sym) {
    
}