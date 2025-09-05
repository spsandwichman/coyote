#ifndef FE_MIR_H
#define FE_MIR_H

#include "iron/iron.h"

typedef struct FeMirSection FeMirSection;

typedef struct {
    FeMirSection* section; // if null, this symbol is absolute
    FeCompactStr name;
    FeSymbolBinding bind;
    FeSymbolKind kind;
    u64 value; // offset from section it's defined in
} FeMirSymbol;

typedef enum : u8 {
    FE_MIRN_INVALID = 0,
    FE_MIRN_ALIGN,
    FE_MIRN_LABEL,
    FE_MIRN_LOCAL_LABEL,

    FE_MIRN_D8,
    FE_MIRN_D16,
    FE_MIRN_D32,
    FE_MIRN_D64,

    FE_MIRN_ZEROES,
    FE_MIRN_BYTES,

    // must be handled by the target encoder
    FE_MIRN_TARGET_INST,
} FeMirNodeKind;

typedef union {
    u32 align;
    FeMirSymbol* label;
    // offset from label's position
    u64 local_label;
    // raw data
    u8  d8;
    u16 d16;
    u32 d32;
    u64 d64;

    u64 zeroes_len;
    struct {
        u32 start; // index into intern bytes array kept by the object
        u32 len;
    } bytes;

    // index into target specific data
    void* target_specific;
} FeMirNode;

typedef enum : u8 {
    FE_MIR_SECTION_EXEC  = 1 << 0,
    FE_MIR_SECTION_READ  = 1 << 1,
    FE_MIR_SECTION_WRITE = 1 << 2,
    FE_MIR_SECTION_BSS   = 1 << 3, // initialized to zero
} FeMirSectionFlags;

typedef struct FeMirSection {
    FeCompactStr name;

    FeMirSectionFlags flags;

    // target-dependent relocation list
    void* relocs;
    u32 reloc_len;
    u32 reloc_cap;

    FeMirNode* mir;
    FeMirNodeKind* mir_kind;
} FeMirSection;

typedef enum : u8 {
    // standard object file
    FE_MIR_OBJ_RELOCATABLE,
    // executable file
    FE_MIR_OBJ_EXECUTABLE,
    // shared library
    FE_MIR_OBJ_SHARED,
} FeMirObjectKind;

typedef struct {
    FeMirObjectKind kind;

    FeMirSection* sections;

    FeMirSymbol* symbols;
    u32 entry_symbol; // only relevant for executable objects

    u8* intern_bytes;
    u32 bytes_len;
    u32 bytes_cap;
} FeMirObject;

#endif // FE_MIR_H