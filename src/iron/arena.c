#include "iron/iron.h"

typedef struct Fe__ArenaChunk Fe__ArenaChunk;
typedef struct FeArena {
    Fe__ArenaChunk* top;
} FeArena;

#define ARENA_CHUNK_DATA_SIZE 32768
typedef struct Fe__ArenaChunk {
    Fe__ArenaChunk* prev;
    usize used;
    u8 data[ARENA_CHUNK_DATA_SIZE];
} Fe__ArenaChunk;

// assume align is a power of two
static inline uintptr_t align_forward(uintptr_t ptr, uintptr_t align) {
    return (ptr + align - 1) & ~(align - 1);
}

// return nullptr if cannot allocate
static void* chunk_alloc(Fe__ArenaChunk* ch, usize size, usize align) {
    if (align_forward(ch->used, align) + size > ARENA_CHUNK_DATA_SIZE) {
        return nullptr;
    }
    ch->used = align_forward(ch->used, align);
    void* mem = &ch->data[ch->used];
    ch->used += size;
    return mem;
}