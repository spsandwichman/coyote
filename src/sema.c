#include "front.h"

Analyzer an;

#define type_node(t) (&an.types.nodes[t])
#define slotsof(T) (sizeof(T) / sizeof(an.types.nodes[0]))
#define as_type(T, x) ((T*)type_node(x))

TypeHandle type_alloc_slots(usize n, bool align_64) {
    if (an.types.len + n >= an.types.cap) {
        an.types.cap *= 2;
        an.types.cap += n;
        an.types.nodes = realloc(an.types.nodes, an.types.cap);
    }
    if (align_64 && (an.types.len & 1)) {
        an.types.len++; // 
    }
    TypeHandle t = an.types.len;
    if (an.types.len + n > TYPE_MAX) {
        // TODO turn this into a user error instead of an ICE
        crash("type node graph exceeded %d slots", TYPE_MAX);
    }
    an.types.len += n;

    memset(&an.types.nodes[t], 0, n * sizeof(an.types.nodes[0]));
    return t;
}

#define is_align64(T) (alignof(T) == 8)

static TypeHandle create_pointer(TypeHandle to) {
    TypeHandle ptr = type_alloc_slots(slotsof(TypeNodePointer), is_align64(TypeNodePointer));
    type_node(ptr)->kind = TYPE_POINTER;
    ((TypeNodePointer*)type_node(ptr))->subtype = to;
    return ptr;
}

TypeHandle type_new_pointer(TypeHandle to) {
    // try to avoid creating a new type if possible
    if (to < _TYPE_SIMPLE_END) {
        return to * slotsof(TypeNodePointer) + _TYPE_SIMPLE_END;
    }
    u8 to_kind = type_node(to)->kind;
    if (to_kind == TYPE_STRUCT || to_kind == TYPE_UNION) {
        return as_type(TypeNodeRecord, to)->pointer_cache;
    }
    // pointer isn't cached, force its creation
    return create_pointer(to);
}

TypeHandle type_new_array(TypeHandle to, u64 len) {
    TypeHandle array = type_alloc_slots(slotsof(TypeNodeArray));
    as_type(TypeNodeArray, array)->kind = TYPE_ARRAY;
    as_type(TypeNodeArray, array)->subtype = to;
    as_type(TypeNodeArray, array)->len = len;
    return array;
}

TypeHandle type_new_record(u8 kind, u16 num_fields) {
    TypeHandle record = type_alloc_slots(
        slotsof(TypeNodeRecord) + // base node
        slotsof(((TypeNodeRecord*)0)->fields[0]) * num_fields // fields
    );
    as_type(TypeNodeRecord, record)->kind = kind;
    as_type(TypeNodeRecord, record)->len = num_fields;
    TypeHandle ptr = create_pointer(record);
    as_type(TypeNodeRecord, record)->pointer_cache = ptr;
    return record;
}

TypeHandle type_new_function(u8 kind, u16 num_params) {
    bool variadic = kind == TYPE_VARIADIC_FNPTR;
    TypeHandle record;
    if (variadic) {
        record = type_alloc_slots(
            slotsof(TypeNodeFunction) + // base node
            slotsof(((TypeNodeFunction*)0)->params[0]) * num_params // params
        );
        as_type(TypeNodeFunction, record)->kind = kind;
        as_type(TypeNodeFunction, record)->len = num_params;
    } else {
        record = type_alloc_slots(
            slotsof(TypeNodeVariadicFunction) + // base node
            slotsof(((TypeNodeVariadicFunction*)0)->params[0]) * num_params // params
        );
        as_type(TypeNodeVariadicFunction, record)->kind = kind;
        as_type(TypeNodeVariadicFunction, record)->len = num_params;
    }
    return record;
}

TypeHandle type_new_enum(u8 kind, u16 num_variants) {
    bool is_64 = kind == TYPE_ENUM64;

    TypeHandle e;
    if (is_64) {
        e = type_alloc_slots(
            slotsof(TypeNodeEnum64) +
            slotsof(((TypeNodeEnum64*)0)->variants[0]) * num_variants
        );
        as_type(TypeNodeEnum64, e)->kind = TYPE_ENUM64;
        as_type(TypeNodeEnum64, e)->len = num_variants;
    } else {
        e = type_alloc_slots(
            slotsof(TypeNodeEnum) +
            slotsof(((TypeNodeEnum*)0)->variants[0]) * num_variants
        );
        as_type(TypeNodeEnum, e)->kind = TYPE_ENUM64;
        as_type(TypeNodeEnum, e)->len = num_variants;
    }
}

void type_init() {
    an.types.cap = 128;
    an.types.nodes = malloc(sizeof(an.types.nodes[0]) * an.types.cap);
    an.types.len = 0;

    for_range(i, 0, _TYPE_SIMPLE_END) {
        an.types.nodes[an.types.len++].kind = i;
    }

    for_range(i, 0, _TYPE_SIMPLE_END) {
        TypeHandle ptr = create_pointer(i);
        assert(type_new_pointer(i) == ptr);
    }
}

static u32 base_sizes[] = {
    [TYPE_VOID] = 0,
    [TYPE_I8] = 1,
    [TYPE_U8] = 1,
    [TYPE_I16] = 2,
    [TYPE_U16] = 2,
    [TYPE_I32] = 4,
    [TYPE_U32] = 4,
    [TYPE_I64] = 8,
    [TYPE_U64] = 8,
};

u32 type_size(TypeHandle t) {
    if (t < _TYPE_SIMPLE_END) {
        return base_sizes[t];
    }

    u8 kind = type_node(t)->kind;
    crash("todo");
    return 0;
}