#include "iron/iron.h"

#define SYMTAB_TOMBSTONE ((void*)0xDEADBEEFBADF00D)

typedef struct Entry {
    FeCompactStr name;
    FeSymbol* sym;
} Entry;

typedef struct FeSymTable {
    Entry* vals;
    u32 len;
    u32 cap;
} FeSymTable;