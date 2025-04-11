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

FeInstPool fe_ipool_new() {
    FeInstPool pool;
    pool.front = ipool_new_chunk();
    for_n(i, 0, FE_IPOOL_FREE_SPACES_LEN) {
        pool.free_spaces[i] = NULL;
    }
    return pool;
}

FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size) {
    if (extra_size > FE_INST_EXTRA_MAX_SIZE)  {
        fe_runtime_crash("extra size > max size");
    }

    usize extra_slots = extra_size / sizeof(usize);
    usize node_slots = extra_slots + sizeof(FeInst) / 8;

    // check if there's any reusable slots.
    for_n(i, extra_slots, FE_INST_EXTRA_MAX_SIZE / sizeof(usize) + 1) {
        if (pool->free_spaces[i] != NULL) {
            // pop from slot list
            FeInst* inst = (FeInst*)pool->free_spaces[i];
            pool->free_spaces[i] = pool->free_spaces[i]->next;
            return inst;
        }
    }

    // try to allocate on the front block
    if (pool->front->used + node_slots <= IPOOL_CHUNK_DATA_SIZE) {
        // allocate on this block!
        FeInst* inst = (FeInst*)&pool->front->data[pool->front->used];
        pool->front->used += node_slots;
        return inst;
    }

    // TODO see if the rest of the space in this block can be
    // converted into a free space. not a huge concern but just a small thing

    // we need to make a new block and allocate on this.
    FeInstPoolChunk* new_chunk = ipool_new_chunk();
    new_chunk->next = pool->front;
    pool->front = new_chunk;
    new_chunk->used += node_slots;
    return (FeInst*) &new_chunk->data;
}

void fe_ipool_free(FeInstPool* pool, FeInst* inst) {
    // set inst extra memory to zero
    usize* extra = fe_extra(inst);
    usize extra_size = fe_inst_extra_size(inst->kind);
    memset(extra, 0, extra_size);

    // maximize slots
    usize size_class = extra_size / sizeof(pool->front->data[0]);
    while (extra[size_class] == 0 && size_class < FE_IPOOL_FREE_SPACES_LEN) {
        size_class += 1;
    }

    // add to free list
    FeInstPoolFreeSpace* free_space = (FeInstPoolFreeSpace*)inst;
    free_space->next = pool->free_spaces[size_class];
    pool->free_spaces[size_class] = free_space;
}