#include "iron/iron.h"
#include <string.h>

typedef struct Fe__ArenaChunk Fe__ArenaChunk;
typedef struct FeArena {
    Fe__ArenaChunk* top;
} FeArena;

#define ARENA_CHUNK_DATA_SIZE 32768
typedef struct Fe__ArenaChunk {
    Fe__ArenaChunk* prev;
    Fe__ArenaChunk* next;
    usize used;
    u8 data[ARENA_CHUNK_DATA_SIZE];
} Fe__ArenaChunk;

// assume align is a power of two
static inline uintptr_t align_forward(uintptr_t ptr, uintptr_t align) {
    return (ptr + align - 1) & ~(align - 1);
}

// return nullptr if cannot allocate
static void* chunk_alloc(Fe__ArenaChunk* ch, usize size, usize align) {
    usize new_used = align_forward(ch->used, align);
    if (new_used + size > ARENA_CHUNK_DATA_SIZE) {
        return nullptr;
    }
    ch->used = new_used;
    void* mem = &ch->data[ch->used];
    ch->used += size;
    return mem;
}

void fe_arena_init(FeArena* arena) {

}

void fe_arena_destroy(FeArena* arena) {
    Fe__ArenaChunk* chunk = arena->top;
    // destroy any saved blocks above
    // for (Fe__ArenaChunk* ch = chunk, *next = ch->next; ch != nullptr; ch = next, next = next->next) {

    // }
}

void* fe_arena_alloc(FeArena* arena, usize size, usize align) {
    void* mem = chunk_alloc(arena->top, size, align);
    if (mem) {
        return mem;
    }

    Fe__ArenaChunk* new_chunk = arena->top->next;
    if (new_chunk == NULL) {
        fe_malloc(sizeof(*new_chunk));
        new_chunk->prev = arena->top;
        new_chunk->next = nullptr;
        arena->top->next = new_chunk;
    }
    new_chunk->used = 0;
    arena->top = new_chunk;
    mem = chunk_alloc(new_chunk, size, align);
    if (mem) {
        return mem;
    }
    fe_runtime_crash("unable to arena-alloc size %zu align %zu", size, align);
}