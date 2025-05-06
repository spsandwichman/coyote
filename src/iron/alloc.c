#include <stdlib.h>
#include <string.h>

#include "iron.h"

static u8 extra_size_table[_FE_INST_END] = {
    // default error value
    [0 ... _FE_INST_END - 1] = 255,

    [FE_BOOKEND] = sizeof(FeInstBookend),
    [FE_PROJ ... FE_MACH_PROJ] = sizeof(FeInstProj),
    [FE_CONST] = sizeof(FeInstConst),
    [FE_PARAM] = sizeof(FeInstParam),
    [FE_IADD ... FE_FREM] = sizeof(FeInstBinop),
    [FE_MOV ... FE_F2I] = sizeof(FeInstUnop),

    [FE_LOAD ... FE_LOAD_VOLATILE] = sizeof(FeInstLoad),
    [FE_STORE ... FE_STORE_VOLATILE] = sizeof(FeInstStore),
    [FE_CASCADE_UNIQUE ... FE_MACH_REG] = 0,

    [FE_BRANCH] = sizeof(FeInstBranch),
    [FE_JUMP] = sizeof(FeInstJump),
    [FE_RETURN] = sizeof(FeInstReturn),
    
    [FE_CALL_DIRECT] = sizeof(FeInstCallDirect),
    [FE_CALL_INDIRECT] = sizeof(FeInstCallIndirect),

    [FE_MACH_STACK_SPILL] = sizeof(FeMachStackSpill),
    [FE_MACH_STACK_RELOAD] = sizeof(FeMachStackReload),
};

void fe__load_extra_size_table(usize start_index, u8* table, usize len) {
    memcpy(&extra_size_table[start_index], table, sizeof(table[0]) * len);
}

usize fe_inst_extra_size_unsafe(FeInstKind kind) {
    u8 size = 255;
    if (kind < _FE_INST_END) {
        size = extra_size_table[kind];
    }

    return (usize) size;
}

usize fe_inst_extra_size(FeInstKind kind) {
    u8 size = fe_inst_extra_size_unsafe(kind);
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

void fe_ipool_init(FeInstPool* pool) {
    pool->top = ipool_new_chunk();
    for_n(i, 0, FE_IPOOL_FREE_SPACES_LEN) {
        pool->free_spaces[i] = nullptr;
    }
}

static FeInst* init_inst(FeInst* inst) {
    inst->kind = 0xFF;
    inst->flags = 0;
    inst->ty = FE_TY_VOID;
    inst->next = nullptr;
    inst->prev = nullptr;
    inst->vr_out = FE_VREG_NONE;
    return inst;
}

FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size) {
    if (extra_size > FE_INST_EXTRA_MAX_SIZE)  {
        fe_runtime_crash("extra size > max size");
    }

    usize extra_slots = extra_size / sizeof(usize);
    usize node_slots = extra_slots + sizeof(FeInst) / 8;

    // check if there's any reusable slots.
    // THE VOICESSSSSS
    for_n(i, extra_slots, FE_INST_EXTRA_MAX_SIZE / sizeof(usize) + 1) {
        if (pool->free_spaces[i] != nullptr) {
            // pop from slot list
            FeInst* inst = (FeInst*)pool->free_spaces[i];
            pool->free_spaces[i] = pool->free_spaces[i]->next;
            return init_inst(inst);
        }
    }

    // try to allocate on the front block
    if (pool->top->used + node_slots <= IPOOL_CHUNK_DATA_SIZE) {
        // allocate on this block!
        FeInst* inst = (FeInst*)&pool->top->data[pool->top->used];
        pool->top->used += node_slots;
        return init_inst(inst);
    } 

    // TODO see if the rest of the space in this block can be
    // converted into a free space. not a huge concern but just a small thing

    // we need to make a new block and allocate on this.
    FeInstPoolChunk* new_chunk = ipool_new_chunk();
    new_chunk->next = pool->top;
    pool->top = new_chunk;
    new_chunk->used += node_slots;
    FeInst* inst = (FeInst*) &new_chunk->data;
    return init_inst(inst);
}

void fe_ipool_free(FeInstPool* pool, FeInst* inst) {
    // set inst extra memory to zero
    usize* extra = fe_extra(inst);
    usize extra_size = fe_inst_extra_size(inst->kind);
    memset(extra, 0, extra_size);

    // reclaim slots
    usize size_class = extra_size / sizeof(pool->top->data[0]);
    while (size_class < FE_IPOOL_FREE_SPACES_LEN - 1 && extra[size_class] == 0) {
        size_class += 1;
    }

    // add to free list
    FeInstPoolFreeSpace* free_space = (FeInstPoolFreeSpace*)inst;
    free_space->next = pool->free_spaces[size_class];
    pool->free_spaces[size_class] = free_space;
}

// "free" the memory without actually giving it back to the allocator
// return the amount of usable space available
usize fe_ipool_free_manual(FeInstPool* pool, FeInst* inst) {
    // set inst extra memory to zero
    usize* extra = fe_extra(inst);
    usize extra_size = fe_inst_extra_size(inst->kind);
    memset(extra, 0, extra_size);

    // reclaim slots
    usize size_class = extra_size / sizeof(pool->top->data[0]);
    while (size_class < FE_IPOOL_FREE_SPACES_LEN - 1 && extra[size_class] == 0) {
        size_class += 1;
    }

    return size_class * sizeof(pool->top->data[0]) + sizeof(FeInst);
}

void fe_ipool_destroy(FeInstPool* pool) {
    FeInstPoolChunk* top = pool->top;
    while (top != nullptr) {
        FeInstPoolChunk* this = top;
        top = top->next;
        fe_free(this);
    } 
    *pool = (FeInstPool){0};
}