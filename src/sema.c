#include "front.h"

#define TG_INITIAL_SIZE 128

TypeGraph tg;


TypeHandle type_alloc_slots(isize n) {
    if (tg.len + n >= tg.cap) {
        tg.cap *= 2;
        tg.cap += n;
        tg.nodes = realloc(tg.nodes, tg.cap);
    }
    TypeHandle t = tg.len;
    tg.len += n;
    
    memset(&tg.nodes[t], 0, n * sizeof(tg.nodes[0]));
    return t;
}

TypeNode* type_node(TypeHandle t) {
    return &tg.nodes[t];
}

#define slotsof(T) (sizeof(T) / sizeof(tg.nodes[0]))

static TypeHandle create_pointer(TypeHandle to) {
    TypeHandle ptr = type_alloc_slots(slotsof(TypeNodePointer));
    type_node(ptr)->kind = TYPE_POINTER;
    ((TypeNodePointer*)type_node(ptr))->subtype = to;
    return ptr;
}

TypeHandle type_pointer(TypeHandle to) {
    if (to < _TYPE_SIMPLE_END) {
        return to + _TYPE_SIMPLE_END; // simple type pointer cache
    }
    if (type_node(to)->kind == TYPE_STRUCT) {
        TypeNodeStruct* ts = (TypeNodeStruct*)type_node(to);
        return ts->pointer_cache;

        // if (ts->pointer_cache == 0) {
        //     TypeHandle ptr = create_pointer(to);
        //     // update ts pointer, cause it might be invalid after create_pointer
        //     ts = (TypeNodeStruct*)type_node(to);
        //     ts->pointer_cache = ptr;
        // }
        // return ts->pointer_cache;
    }
    return create_pointer(to);
}

TypeHandle type_array(TypeHandle to, u64 len) {
    TypeHandle array = type_alloc_slots(slotsof(TypeNodeArray));
}

void type_init() {
    tg.nodes = malloc(sizeof(tg.nodes[0]) * TG_INITIAL_SIZE);
    tg.cap = TG_INITIAL_SIZE;
    tg.len = 0;

    for_range(i, 0, _TYPE_SIMPLE_END) {
        tg.nodes[tg.len++].kind = i;
    }

    for_range(i, 0, _TYPE_SIMPLE_END) {
        TypeHandle ptr = create_pointer(i);
    }
}