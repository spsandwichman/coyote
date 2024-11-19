#include "front.h"

Analyzer an;

#define type_node(t) (&an.types.nodes[t])
#define slotsof(T) (sizeof(T) / sizeof(an.types.nodes[0]))
#define as_type(T, x) ((T*)type_node(x))

TypeHandle type_alloc_slots(usize n, bool align_64) {
    if (align_64 && (an.types.len & 1)) {
        an.types.len++; // 
    }
    if (an.types.len + n >= an.types.cap) {
        an.types.cap *= 2;
        an.types.cap += n;
        an.types.nodes = realloc(an.types.nodes, an.types.cap);
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
    as_type(TypeNodePointer, ptr)->subtype = to;
    return ptr;
}

TypeHandle type_new_alias(TypeHandle to) {
    TypeHandle alias = type_alloc_slots(slotsof(TypeNodeAlias), is_align64(TypeNodeAlias));
    type_node(alias)->kind = TYPE_ALIAS;
    as_type(TypeNodeAlias, alias)->subtype = to;
    return alias;
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
    TypeHandle array = type_alloc_slots(slotsof(TypeNodeArray), is_align64(TypeNodeArray));
    as_type(TypeNodeArray, array)->kind = TYPE_ARRAY;
    as_type(TypeNodeArray, array)->subtype = to;
    as_type(TypeNodeArray, array)->len = len;
    return array;
}

TypeHandle type_new_record(u8 kind, u16 num_fields) {
    TypeHandle record = type_alloc_slots(
        slotsof(TypeNodeRecord) + // base node
        slotsof(((TypeNodeRecord*)0)->fields[0]) * num_fields, // fields
        is_align64(TypeNodeRecord)
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
            slotsof(((TypeNodeFunction*)0)->params[0]) * num_params, // params
            is_align64(TypeNodeFunction)
        );
        as_type(TypeNodeFunction, record)->kind = kind;
        as_type(TypeNodeFunction, record)->len = num_params;
    } else {
        record = type_alloc_slots(
            slotsof(TypeNodeVariadicFunction) + // base node
            slotsof(((TypeNodeVariadicFunction*)0)->params[0]) * num_params, // params
            is_align64(TypeNodeFunction)
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
            slotsof(((TypeNodeEnum64*)0)->variants[0]) * num_variants,
            is_align64(TypeNodeEnum64)
        );
        as_type(TypeNodeEnum64, e)->kind = TYPE_ENUM64;
        as_type(TypeNodeEnum64, e)->len = num_variants;
    } else {
        e = type_alloc_slots(
            slotsof(TypeNodeEnum) +
            slotsof(((TypeNodeEnum*)0)->variants[0]) * num_variants,
            is_align64(TypeNodeEnum)
        );
        as_type(TypeNodeEnum, e)->kind = TYPE_ENUM64;
        as_type(TypeNodeEnum, e)->len = num_variants;
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

static usize  FNV_1a(char* raw, usize len) {
    const usize FNV_OFFSET = 14695981039346656037ull;
    const usize FNV_PRIME = 1099511628211ull;

    usize hash = FNV_OFFSET;
    for_urange(i, 0, len) {
        hash ^= (usize)(u8)(raw[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

static bool strpool_eq(Index s, char* key, usize key_len) {
    char* str = &an.strings.chars[s];
    // this is probably going to make valgrind mad
    // too bad!
    return strncmp(str, key, key_len) == 0 && str[key_len] == '\0';
}

static Index strpool_alloc(char* raw, usize len) {
    if (an.strings.len + len >= an.strings.cap) {
        an.strings.cap *= 2;
        an.strings.cap += len + 1;
        an.strings.chars = realloc(an.strings.chars, an.strings.cap);
    }

    Index str = an.strings.len;
    memcpy(&an.strings.len, raw, len);
    an.strings.chars[an.strings.len + len] = '\0'; // NUL terminated
    an.strings.len += len + 1;

    return str;
}


// this parameter may be invalidated by additions to the entity buffer
Entity* entity_get(EntityHandle e) {
    return &an.entities.items[e];
}

#define ENTITY_TABLE_MAX_SEARCH 5

EntityHandle etable_search(EntityTable* tbl, char* raw, usize len) {
    if (tbl == NULL) return 0; // not found

    usize hash = FNV_1a(raw, len);
    for_range(i, 0, ENTITY_TABLE_MAX_SEARCH) {
        usize index = (hash + i) % tbl->cap;
        if (tbl->entities[index] == 0) {
            break; // this slot is empty, so nothing is after this
        }
        if (strpool_eq(tbl->name_strings[index], raw, len)) {
            return tbl->entities[index];
        }
    }

    return etable_search(tbl->parent, raw, len);
}

static inline bool _etbl_put(EntityHandle* entities, Index* name_strings, u32 cap, EntityHandle e, Index name_str, char* raw, usize len) {
    usize hash = FNV_1a(raw, len);
    for_range(i, 0, ENTITY_TABLE_MAX_SEARCH) {
        usize index = (hash + i) % cap;
        if (entities[index] == 0) {
            entities[index] = e;
            name_strings[index] = name_str;
            return true;
        }
    }
    return false;
}

// allocates a name in the strin
void etable_put(EntityTable* tbl, EntityHandle entity, char* raw, usize len) {

    Index name_str = strpool_alloc(raw, len);

    if (_etbl_put(tbl->entities, tbl->name_strings, tbl->cap, entity, name_str, raw, len)) {
        return;
    }

    u32 new_cap = tbl->cap;

    restart: // restart reallocation process

    // if a slot is not found, resize the table
    new_cap *= 2;
    EntityHandle* new_entities = malloc(sizeof(tbl->entities[0]) * new_cap);
    memset(new_entities, 0, sizeof(tbl->entities[0]) * new_cap);
    Index* new_name_strings = malloc(sizeof(tbl->name_strings[0]) * new_cap);
    memset(new_name_strings, 0, sizeof(tbl->name_strings[0]) * new_cap);

    for_range(i, 0, tbl->cap) {
        if (tbl->entities[i] == 0) continue;

        char* str = &an.strings.chars[i];
        usize str_len = strlen(str);

        bool put = _etbl_put(
            new_entities, new_name_strings, new_cap, 
            tbl->entities[i], tbl->name_strings[i],
            str, str_len
        );
        if (!put) {
            free(new_entities);
            free(new_name_strings);
            goto restart;
        }
    }

    tbl->cap = new_cap;
    tbl->entities = new_entities;
    tbl->name_strings = new_name_strings;

    // try putting the current thing again
    etable_put(tbl, entity, raw, len);
}

EntityTable* etable_new(EntityTable* parent, u32 initial_cap) {
    EntityTable* tbl = malloc(sizeof(*tbl));
    tbl->parent = parent;
    tbl->cap = initial_cap;
    tbl->entities = malloc(sizeof(tbl->entities[0]) * initial_cap);
    memset(tbl->entities, 0, sizeof(tbl->entities[0]) * initial_cap);
    tbl->name_strings = malloc(sizeof(tbl->name_strings[0]) * initial_cap);
    memset(tbl->name_strings, 0, sizeof(tbl->name_strings[0]) * initial_cap);

    return tbl;
}

EntityHandle entity_new(EntityTable* tbl) {
    if (an.entities.len == an.entities.cap) {
        an.entities.cap *= 2;
        an.entities.items = realloc(an.entities.items, sizeof(an.entities.items[0]) * an.entities.cap);
    }
    return an.entities.len++;
}

#define expr_as(T, e) ((T*)&an.exprs.items[e])
#define se_new(T) se_alloc_slots(sizeof(T) / sizeof(an.exprs.items[0]))
#define se_new_with(T, slots) se_alloc_slots(sizeof(T) / sizeof(an.exprs.items[0]) + slots)

Index se_alloc_slots(usize slots) {
    if (an.exprs.len + slots >= an.exprs.cap) {
        an.exprs.cap *= 2;
        an.exprs.cap += slots;
        an.exprs.items = realloc(an.exprs.items, an.exprs.cap);
    }
    Index expr = an.exprs.len;
    an.exprs.len += slots;
    memset(&an.exprs.items[expr], 0, sizeof(an.exprs.items[expr]) * slots);
    return expr;
}

void sema_init() {
    // initialize type graph
    an.types.len = 0;
    an.types.cap = 128;
    an.types.nodes = malloc(sizeof(an.types.nodes[0]) * an.types.cap);

    for_range(i, 0, _TYPE_SIMPLE_END) {
        an.types.nodes[an.types.len++].kind = i;
    }

    for_range(i, 0, _TYPE_SIMPLE_END) {
        TypeHandle ptr = create_pointer(i);
        assert(type_new_pointer(i) == ptr);
    }

    // init entity array
    an.entities.len = 0;
    an.entities.cap = 512;
    an.entities.items = malloc(sizeof(an.entities.items[0]) * an.entities.cap);

    // init global level entity table
    an.global = etable_new(NULL, 128);

    // add null entity
    assert(entity_new(an.global) == 0);
    
    // init exprs arena
    an.exprs.len = 0;
    an.exprs.cap = 512;
    an.exprs.items = malloc(sizeof(an.exprs.items[0]) * an.exprs.cap);

    // init stmts arena
    an.stmts.len = 0;
    an.stmts.cap = 128;
    an.stmts.items = malloc(sizeof(an.stmts.items[0]) * an.stmts.cap);

    // init string arena
    an.strings.len = 0;
    an.strings.cap = 512;
    an.strings.chars = malloc(sizeof(an.strings.chars[0]) * an.strings.cap);
}

Analyzer sema_analyze(ParseTree pt, TokenBuf tb) {
    sema_init();
    an.pt = pt;
    an.tb = tb;
}

bool type_is_defined(TypeHandle t) {
    if (t < _TYPE_SIMPLE_END || type_node(t)->kind != TYPE_ALIAS) return true;
    while (type_node(t)->kind != TYPE_ALIAS) {
        t = as_type(TypeNodeAlias, t)->subtype;
        if (t == TYPE_ALIAS_UNDEF) return false;
    }
    return true;
}

TypeHandle sema_ingest_type(EntityTable* tbl, Index pnode) {
    ParseNode* node = parse_node(pnode);
    u8 node_kind = parse_node_kind(pnode);
    switch (node_kind) {
    case PN_TYPE_POINTER:
        TypeHandle subtype = sema_ingest_type(tbl, node->lhs);
        TypeHandle ptr = type_new_pointer(subtype);
        return ptr;
        break;
    case PN_EXPR_IDENT:
        Token t = an.tb.at[node->main_token];
        EntityHandle e = etable_search(tbl, t.raw, t.len);
        if (e == 0) { // entity undefined so far
            // create it and point it to an undefined alias
            TypeHandle type = type_new_alias(TYPE_ALIAS_UNDEF);
            e = entity_new(tbl);
            entity_get(e)->kind = ENTITY_TYPENAME;
            entity_get(e)->type = type;
            etable_put(tbl, e, t.raw, t.len);
            return type;
        }
        if (entity_get(e)->kind == ENTITY_TYPENAME) {
            return entity_get(e)->type;
        }
        report_token(true, &an.tb, node->main_token, "entity is not a type");
    case PN_TYPE_ARRAY:
        TypeHandle subtype = sema_ingest_type(tbl, node->lhs);
        TODO("sema - const eval");
    default:
        break;
    }
}

Index sema_check_expr(EntityTable* tbl, Index pnode) {
    ParseNode* node = parse_node(pnode);
    u8 node_kind = parse_node_kind(pnode);

    switch (node_kind) {
    case PN_EXPR_IDENT:
        
    }
}