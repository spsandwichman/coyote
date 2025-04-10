#include "iron.h"

static const u8 base_extra_size_table[_FE_BASE_INST_MAX] = {
    // default error value
    [0 ... _FE_BASE_INST_MAX - 1] = 255,

    [FE_BOOKEND]                     = sizeof(FeInstBookend),
    [FE_PROJ]                        = sizeof(FeInstProj),
    [FE_IADD ... FE_ASR]             = sizeof(FeInstBinop),
    [FE_MOV ... FE_NEG]              = sizeof(FeInstUnop),

    [FE_LOAD ... FE_LOAD_VOLATILE]   = sizeof(FeInstLoad),
    [FE_STORE ... FE_STORE_VOLATILE] = sizeof(FeInstStore),
    [FE_CASCADE_VOLATILE ... FE_CASCADE_UNIQUE] = 0,
};

usize fe_inst_extra_size_unsafe(u8 kind) {
    u8 size = 255;
    if (kind < _FE_BASE_INST_MAX) {
        size = base_extra_size_table[kind];
    }

    return (usize) size;
}

usize fe_inst_extra_size(u8 kind) {
    u8 size = 255;
    if (kind < _FE_BASE_INST_MAX) {
        size = base_extra_size_table[kind];
    }

    if (size == 255) {
        fe_runtime_crash("invalid inst kind %u", kind);
    }
    return (usize) size;
}

#define IPOOL_CHUNK_DATA_SIZE 4096
struct FeInstPoolChunk {
    FeInstPoolChunk* next;
    usize used;
    usize data[IPOOL_CHUNK_DATA_SIZE];
    // set to non-zero so slot reclaimer doesn't
    // walk off the end of the chunk data on accident
    usize backstop;
};

struct FeInstPoolFreeSpace {
    FeInstPoolFreeSpace* next;
};

static FeInstPoolChunk* ipool_new_chunk() {
    FeInstPoolChunk* chunk = fe_malloc(sizeof(FeInstPoolChunk));
    chunk->backstop = 0xFF;
    return chunk;
}

FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size) {
    if (extra_size > FE_INST_EXTRA_MAX_SIZE)  {
        fe_runtime_crash("extra size > max size");
    }

    usize extra_slots = extra_size / sizeof(usize);

    // check if there's any reusable slots.
    for_n(i, extra_slots, FE_INST_EXTRA_MAX_SIZE / sizeof(usize) + 1) {
        if (pool->free_spaces[i] != NULL) {
            // pop from slot list
            void* inst = pool->free_spaces[i];
            pool->free_spaces[i] = pool->free_spaces[i]->next;
            return inst;
        }
    }

    // try to allocate on the front block
    if (pool->front->used + extra_slots <= IPOOL_CHUNK_DATA_SIZE) {
        // allocate on this block!
        void* inst = &pool->front->data[pool->front->used];
        pool->front->used += extra_slots;
        return inst;
    }

    // TODO see if the rest of the space in this block can be
    // converted into a free space.

    // we need to make a new block and allocate on this.
    FeInstPoolChunk* new_chunk = ipool_new_chunk();
    new_chunk->next = pool->front;
    pool->front = new_chunk;
    new_chunk->used += extra_slots;
    return (FeInst*) &new_chunk->data;
}