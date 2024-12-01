#include "sema.h"
#include "strbuilder.h"

static Analyzer an;

#define slotsof(T) (sizeof(T) / sizeof(an.types.nodes[0]))

static usize align_forward(usize ptr, usize align) {
    if (!is_pow_2(align)) {
        crash("internal: align is not a power of two (got %zu)\n", align);
    }

    return (ptr + align - 1) & ~(align - 1);
}

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

static void _type_name(StringBuilder* sb, TypeHandle t) {
    switch (type_node(t)->kind) {
    case TYPE_VOID: sb_append_c(sb, "VOID"); break;
    case TYPE_I8:   sb_append_c(sb, "BYTE"); break;
    case TYPE_U8:   sb_append_c(sb, "UBYTE"); break;
    case TYPE_I16:  sb_append_c(sb, "INT"); break;
    case TYPE_U16:  sb_append_c(sb, "UINT"); break;
    case TYPE_I32:  sb_append_c(sb, "LONG"); break;
    case TYPE_U32:  sb_append_c(sb, "ULONG"); break;
    case TYPE_I64:  sb_append_c(sb, "QUAD"); break;
    case TYPE_U64:  sb_append_c(sb, "UQUAD"); break;
    // case TYPE_INT_CONSTANT: sb_append_c(sb, type_name(an.max_int)); break;
    case TYPE_INT_CONSTANT: sb_append_c(sb, "<<<int const>>>"); break;
    case TYPE_ALIAS:
    case TYPE_ALIAS_UNDEF: {
        Index str_index = entity_get(type_as(TypeNodeAlias, t)->entity)->ident_string;
        const char* cstr = &an.strings.chars[str_index];
        sb_append_c(sb, cstr);
        break;
        }
    case TYPE_POINTER: {
        sb_append_c(sb, "^");
        _type_name(sb, type_as(TypeNodePointer, t)->subtype);
        break;
        }
    default:
        TODO("unhandled type");
    }
}

const char* type_name(TypeHandle t) {
    switch (t) {
    case TYPE_VOID: return "VOID";
    case TYPE_I8:   return "BYTE";
    case TYPE_U8:   return "UBYTE";
    case TYPE_I16:  return "INT";
    case TYPE_U16:  return "UINT";
    case TYPE_I32:  return "LONG";
    case TYPE_U32:  return "ULONG";
    case TYPE_I64:  return "QUAD";
    case TYPE_U64:  return "UQUAD";
    // case TYPE_INT_CONSTANT: return type_name(an.max_int);
    case TYPE_INT_CONSTANT: return "<int const>";
    default:
        StringBuilder sb;
        sb_init(&sb);
        _type_name(&sb, t);
        char* str = malloc(sb.len + 1);
        sb_write(&sb, str);
        str[sb.len] = '\0';
        sb_destroy(&sb);
        return str;
    }
}

static TypeHandle create_pointer(TypeHandle to) {
    TypeHandle ptr = type_alloc_slots(slotsof(TypeNodePointer), is_align64(TypeNodePointer));
    type_node(ptr)->kind = TYPE_POINTER;
    type_as(TypeNodePointer, ptr)->subtype = to;
    return ptr;
}

TypeHandle type_new_alias_undef(EntityHandle ent) {
    TypeHandle alias = type_alloc_slots(slotsof(TypeNodeAlias), is_align64(TypeNodeAlias));
    type_node(alias)->kind = TYPE_ALIAS_UNDEF;
    type_as(TypeNodeAlias, alias)->subtype = TYPE_VOID;
    type_as(TypeNodeAlias, alias)->entity = ent;
    return alias;
}

bool type_is_complete(TypeHandle t) {
    if (t < _TYPE_SIMPLE_END) return true;
    while (type_node(t)->kind == TYPE_ALIAS) {
        t = type_as(TypeNodeAlias, t)->subtype;
    }
    if (type_node(t)->kind == TYPE_ALIAS_UNDEF) return false;
    if (type_node(t)->kind == TYPE_ARRAY) {
        if (type_as(TypeNodeArray, t)->len == ARRAY_UNKNOWN_LEN) return false;
    }
    return true;
}

TypeHandle type_new_alias(TypeHandle to, EntityHandle ent) {
    TypeHandle alias = type_alloc_slots(slotsof(TypeNodeAlias), is_align64(TypeNodeAlias));
    type_node(alias)->kind = TYPE_ALIAS;
    type_as(TypeNodeAlias, alias)->subtype = to;
    type_as(TypeNodeAlias, alias)->entity = ent;
    return alias;
}

TypeHandle type_new_pointer(TypeHandle to) {
    // try to avoid creating a new type if possible
    if (to < _TYPE_SIMPLE_END) {
        return to * slotsof(TypeNodePointer) + _TYPE_SIMPLE_END;
    }
    u8 to_kind = type_node(to)->kind;
    if (to_kind == TYPE_STRUCT || to_kind == TYPE_UNION) {
        return type_as(TypeNodeRecord, to)->pointer_cache;
    }
    // pointer isn't cached, force its creation
    return create_pointer(to);
}

TypeHandle type_new_array(TypeHandle to, u64 len) {
    TypeHandle array = type_alloc_slots(slotsof(TypeNodeArray), is_align64(TypeNodeArray));
    type_as(TypeNodeArray, array)->kind = TYPE_ARRAY;
    type_as(TypeNodeArray, array)->subtype = to;
    type_as(TypeNodeArray, array)->len = len;
    return array;
}

TypeHandle type_new_record(u8 kind, u16 num_fields) {
    TypeHandle record = type_alloc_slots(
        slotsof(TypeNodeRecord) + // base node
        slotsof(((TypeNodeRecord*)0)->fields[0]) * num_fields, // fields
        is_align64(TypeNodeRecord)
    );
    type_as(TypeNodeRecord, record)->kind = kind;
    type_as(TypeNodeRecord, record)->len = num_fields;
    TypeHandle ptr = create_pointer(record);
    type_as(TypeNodeRecord, record)->pointer_cache = ptr;
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
        type_as(TypeNodeFunction, record)->kind = kind;
        type_as(TypeNodeFunction, record)->len = num_params;
    } else {
        record = type_alloc_slots(
            slotsof(TypeNodeVariadicFunction) + // base node
            slotsof(((TypeNodeVariadicFunction*)0)->params[0]) * num_params, // params
            is_align64(TypeNodeFunction)
        );
        type_as(TypeNodeVariadicFunction, record)->kind = kind;
        type_as(TypeNodeVariadicFunction, record)->len = num_params;
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
        type_as(TypeNodeEnum64, e)->kind = TYPE_ENUM64;
        type_as(TypeNodeEnum64, e)->len = num_variants;
    } else {
        e = type_alloc_slots(
            slotsof(TypeNodeEnum) +
            slotsof(((TypeNodeEnum*)0)->variants[0]) * num_variants,
            is_align64(TypeNodeEnum)
        );
        type_as(TypeNodeEnum, e)->kind = TYPE_ENUM64;
        type_as(TypeNodeEnum, e)->len = num_variants;
    }
}


u32 type_size(TypeHandle t) {
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
    if (t < _TYPE_SIMPLE_END) {
        return base_sizes[t];
    }
    u8 kind = type_node(t)->kind;
    switch (kind) {
    case TYPE_POINTER: return base_sizes[an.max_uint];
    }
    TODO("todo");
    return 0;
}

u32 type_align(TypeHandle t) {
    static u32 base_aligns[] = {
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
    if (t < _TYPE_SIMPLE_END) {
        return base_aligns[t];
    }
    u8 kind = type_node(t)->kind;
    switch (kind) {
    case TYPE_POINTER: return base_aligns[an.max_uint];
    }
    TODO("todo");
    return 0;
}

TypeHandle type_unalias(TypeHandle t) {
    while (type_as(TypeNode, t)->kind == TYPE_ALIAS) {
        t = type_as(TypeNodeAlias, t)->subtype;
    }
    return t;
}

bool type_is_signed(TypeHandle t) {
    assert(t < _TYPE_SIMPLE_END);
    return t & 1;
}

bool type_is_concrete_integer(TypeHandle t) {
    return TYPE_I8 <= t && t <= TYPE_U64;
}

bool type_is_integer(TypeHandle t) {
    return TYPE_I8 <= t && t <= TYPE_INT_CONSTANT;
}

bool type_is_integral(TypeHandle t) {
    u8 kind = type_node(t)->kind;
    return (TYPE_I8 <= t && t <= TYPE_INT_CONSTANT)
        || kind == TYPE_POINTER
        || kind == TYPE_ENUM
        || kind == TYPE_ENUM64;
}

bool type_is_equal(TypeHandle from, TypeHandle to) {
    // simple equality
    if (from == to) return true;
    // peel off aliases
    while (type_node(from)->kind == TYPE_ALIAS) {
        from = type_as(TypeNodeAlias, from)->subtype;
    }
    while (type_node(to)->kind == TYPE_ALIAS) {
        to = type_as(TypeNodeAlias, to)->subtype;
    }
    // simple equality
    if (from == to) return true;
    if (type_node(from)->kind != type_node(to)->kind) return false;

    switch (type_node(from)->kind) {
    case TYPE_POINTER: {
        TypeHandle from_subtype = type_as(TypeNodePointer, from)->subtype;
        TypeHandle to_subtype = type_as(TypeNodePointer, to)->subtype;
        return type_is_equal(from_subtype, to_subtype);
    }
    case TYPE_ALIAS_UNDEF: 
        return false; // aliases that are not EXACTLY equal arent equal at all
    default:
        TODO("");
    }
    return false;
}

bool type_can_cast(TypeHandle from, TypeHandle to, bool explicit) {

    // simple equality
    if (from == to) return true;

    // peel off aliases
    while (type_node(from)->kind == TYPE_ALIAS) {
        from = type_as(TypeNodeAlias, from)->subtype;
    }
    while (type_node(to)->kind == TYPE_ALIAS) {
        to = type_as(TypeNodeAlias, to)->subtype;
    }
    
    // cannot cast to incomplete types
    if (!type_is_complete(to) || !type_is_complete(from)) {
        return false;
    }
    // simple equality
    if (from == to) return true;

    if (from == TYPE_INT_CONSTANT) {
        // int constant can implicit cast to any integer
        if (type_is_concrete_integer(to)) return true;
        // implicit int constant -> pointer
        if (type_node(to)->kind == TYPE_POINTER) return true;
    }



    if (type_is_concrete_integer(to) && type_is_concrete_integer(from)) {
        // cannot cast between signedness
        if (type_is_signed(to) != type_is_signed(from)) return false;
        // concrete integers can cast to any other with the same signedness
        return true;
    }

    if (type_node(from)->kind == TYPE_POINTER) {
        if (type_node(to)->kind == TYPE_POINTER) {
            TypeHandle from_subtype = type_as(TypeNodePointer, from)->subtype;
            TypeHandle to_subtype = type_as(TypeNodePointer, to)->subtype;

            // ^T -> ^VOID || ^VOID -> ^T
            if (from_subtype == TYPE_VOID || to_subtype == TYPE_VOID) {
                return true;
            }

            // CAST ^U TO ^V
            // for any U and V
            if (explicit) {
                return true;
            }
        } else if (to == an.max_uint) {
            return true; // implicit ^T -> UWORD
        }
    }

    // everything past this must be explicit
    if (!explicit) return false;

    // array -> array casts
    if (type_node(from)->kind == TYPE_ARRAY && type_node(to)->kind == TYPE_ARRAY) {

        // check if from (by transitive, 'to' as well) has an unknown length
        if (type_as(TypeNodeArray, from)->len == ARRAY_UNKNOWN_LEN) return false;
        // check if the lengths arent the same
        if (type_as(TypeNodeArray, to)->len != type_as(TypeNodeArray, from)->len) return false;

        // U[N] -> V[N] as long as
        // sizeof(U) == sizeof(V) && alignof(U) == alignof(V)
        TypeHandle from_subtype = type_as(TypeNodeArray, from)->subtype;
        TypeHandle to_subtype = type_as(TypeNodeArray, to)->subtype;
        return type_size(to_subtype) == type_size(to_subtype)
            && type_align(to_subtype) == type_align(to_subtype);
    }

    return false;
}

static usize FNV_1a(char* raw, usize len) {
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
    if (str >= MAX_STRING) {
        crash("too many strings!");
    }
    memcpy(&an.strings.chars[an.strings.len], raw, len);
    an.strings.chars[an.strings.len + len] = '\0'; // NUL terminated

    an.strings.len += len + 1;

    return str;
}

Entity* entity_get(EntityHandle e) {
    return &an.entities.items[e];
}

#define ENTITY_TABLE_MAX_SEARCH 5

EntityHandle etable_search(EntityTable* tbl, char* raw, usize len) {
    if (tbl == NULL) return 0; // not found

    // for_range(i, 0, tbl->cap) {
    //     Index str = tbl->name_strings[i];
    //     printf("[%d] '%s' -> %d\n", i, str == 0 ? NULL : &an.strings.chars[str], tbl->entities[i]);
    // }

    usize hash = FNV_1a(raw, len);
    for_range(i, 0, ENTITY_TABLE_MAX_SEARCH) {
        usize index = (hash + i) % tbl->cap;
        if (tbl->entities[index] == 0) {
            break; // this slot is empty, so nothing is after this
        }
        if (strpool_eq(entity_get(tbl->entities[index])->ident_string, raw, len)) {
            return tbl->entities[index];
        }
    }

    return etable_search(tbl->parent, raw, len);
}

static inline bool _etbl_put(EntityHandle* entities, u32 cap, EntityHandle e, char* raw, usize len) {
    usize hash = FNV_1a(raw, len);
    for_range(i, 0, ENTITY_TABLE_MAX_SEARCH) {
        usize index = (hash + i) % cap;
        if (entities[index] == 0) {
            entities[index] = e;
            return true;
        }
    }
    return false;
}

// allocates a name in the string pool
void etable_put(EntityTable* tbl, EntityHandle entity, char* raw, usize len) {

    Index name_str = strpool_alloc(raw, len);
    entity_get(entity)->ident_string = name_str;

    if (_etbl_put(tbl->entities, tbl->cap, entity, raw, len)) {
        return;
    }

    u32 new_cap = tbl->cap;

    restart: // restart reallocation process

    // if a slot is not found, resize the table
    new_cap *= 2;
    EntityHandle* new_entities = malloc(sizeof(tbl->entities[0]) * new_cap);
    memset(new_entities, 0, sizeof(tbl->entities[0]) * new_cap);

    for_range(i, 0, tbl->cap) {
        if (tbl->entities[i] == 0) continue;

        char* str = &an.strings.chars[i];
        usize str_len = strlen(str);

        bool put = _etbl_put(
            new_entities, new_cap, 
            tbl->entities[i],
            str, str_len
        );
        if (!put) {
            free(new_entities);
            goto restart;
        }
    }

    tbl->cap = new_cap;
    tbl->entities = new_entities;

    // try putting the current thing again
    etable_put(tbl, entity, raw, len);
}

EntityTable* etable_new(EntityTable* parent, u32 initial_cap) {
    EntityTable* tbl = malloc(sizeof(*tbl));
    tbl->parent = parent;
    tbl->cap = initial_cap;
    tbl->entities = malloc(sizeof(tbl->entities[0]) * initial_cap);
    memset(tbl->entities, 0, sizeof(tbl->entities[0]) * initial_cap);

    return tbl;
}

EntityHandle entity_new() {
    if (an.entities.len == an.entities.cap) {
        an.entities.cap *= 2;
        an.entities.items = realloc(an.entities.items, sizeof(an.entities.items[0]) * an.entities.cap);
    }
    return an.entities.len++;
}

#define expr_as(T, e) ((T*)&an.exprs.items[e])
#define expr_(e) ((SemaExpr*)&an.exprs.items[e])
#define expr_new(T) expr_alloc_slots(sizeof(T) / sizeof(an.exprs.items[0]))
#define expr_new_with(T, slots) expr_alloc_slots(sizeof(T) / sizeof(an.exprs.items[0]) + slots)

Index expr_alloc_slots(usize slots) {
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

#define stmt_as(T, e) ((T*)&an.stmts.items[e])
#define stmt_(e) ((SemaStmt*)&an.stmts.items[e])
#define stmt_new(T) stmt_alloc_slots(sizeof(T) / sizeof(an.stmts.items[0]))
#define stmt_new_with(T, slots) stmt_alloc_slots(sizeof(T) / sizeof(an.stmts.items[0]) + slots)

Index stmt_alloc_slots(usize slots) {
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
    assert(entity_new() == 0);
    
    // init exprs arena
    an.exprs.len = 0;
    an.exprs.cap = 512;
    an.exprs.items = malloc(sizeof(an.exprs.items[0]) * an.exprs.cap);

    // init stmts arena
    an.stmts.len = 0;
    an.stmts.cap = 256;
    an.stmts.items = malloc(sizeof(an.stmts.items[0]) * an.stmts.cap);

    // init string arena
    an.strings.len = 1;
    an.strings.cap = 512;
    an.strings.chars = malloc(sizeof(an.strings.chars[0]) * an.strings.cap);

    an.max_int  = TYPE_I64; // assume 64 bit at them moment
    an.max_uint = TYPE_U64;
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
            e = entity_new();
            entity_get(e)->kind = ENTKIND_TYPENAME;
            TypeHandle type = type_new_alias_undef(e);
            entity_get(e)->type = type;
            etable_put(tbl, e, t.raw, t.len);
            return type;
        }
        if (entity_get(e)->kind == ENTKIND_TYPENAME) {
            return entity_get(e)->type;
        }
        report_token(true, &an.tb, node->main_token, "entity is not a type");
        break;
    case PN_TYPE_ARRAY:
        subtype = sema_ingest_type(tbl, node->lhs);
        if (!type_is_complete(subtype)) {
            report_token(true, &an.tb, node->main_token, "incomplete type cannot be used");
        } 
        TODO("const eval");
    case PN_TYPE_VOID:  return TYPE_VOID;
    case PN_TYPE_BYTE:  return TYPE_U8;
    case PN_TYPE_UBYTE: return TYPE_I8;
    case PN_TYPE_INT:   return TYPE_U16;
    case PN_TYPE_UINT:  return TYPE_I16;
    case PN_TYPE_LONG:  return TYPE_U32;
    case PN_TYPE_ULONG: return TYPE_I32;
    case PN_TYPE_QUAD:  return TYPE_U64;
    case PN_TYPE_UQUAD: return TYPE_I64;
    case PN_TYPE_WORD:  return an.max_int;
    case PN_TYPE_UWORD: return an.max_uint;
    }
    crash("unhandled %d", parse_node_kind(pnode));
    return 0;
}

static u64 eval_integer(Index token, char* raw, usize len) {
    if (len == 1) {
        return raw[0] - '0';
    }
    isize value = 0;
    usize base = 10;
    if (raw[0] == '0') switch (raw[1]) {
    case 'x':
    case 'X':
        base = 16;
        raw += 2; 
        len -= 2; 
        break;
    case 'o':
    case 'O':
        base = 8;
        raw += 2; 
        len -= 2; 
        break;
    case 'b':
    case 'B':
        base = 2; 
        raw += 2; 
        len -= 2; 
        break;
    }

    for_range(i, 0, len) {
        char c = raw[i];
        if (c == '_') continue;
        usize cval = 0;
        if ('0' <= c && c <= '9') {
            cval = c - '0';
        } else if ('a' <= c && c <= 'f') {
            cval = c - 'a';
        } else if ('A' <= c && c <= 'F') {
            cval = c - 'A';
        }
        if (cval >= base) {
            report_token(true, &an.tb, token, "'%c' is not a valid base-%d digit", c, base);
        }
        // TODO add overflow check here
        value = value * base + cval;
    }
    return value;
}

// if the value is an int constant, set its type to the TypeHandle and return it
// else, insert a cast
Index sema_insert_implicit_cast(Index value, TypeHandle to) {
    if (expr_(value)->type == TYPE_INT_CONSTANT) {
        expr_(value)->type = to;
        return value;
    }
    Index cast = expr_new(SemaExprUnop);
    expr_as(SemaExprUnop, cast)->base.kind = SE_IMPLICIT_CAST;
    expr_as(SemaExprUnop, cast)->base.type = to;
    expr_as(SemaExprUnop, cast)->sub = value;
    return cast;
}

TypeHandle type_binop_result(TypeHandle lhs, TypeHandle rhs) {
    if (type_node(lhs)->kind == TYPE_POINTER) return lhs;
    if (type_node(rhs)->kind == TYPE_POINTER) return rhs;
    if (lhs == TYPE_INT_CONSTANT) return rhs;
    // if (rhs == TYPE_INT_CONSTANT) return lhs;
    return lhs;
}

Index sema_check_expr(EntityTable* tbl, Index pnode) {
    assert(pnode != 0); // make sure we didn't get a null parse node
    ParseNode* node = parse_node(pnode);
    u8 node_kind = parse_node_kind(pnode);

    switch (node_kind) {
    case PN_EXPR_NULLPTR: {
        Index ptr = expr_new(SemaExprInteger);
        expr_(ptr)->kind = SE_INTEGER;
        expr_(ptr)->parse_node = pnode;
        expr_(ptr)->type = type_new_pointer(TYPE_VOID);
        expr_as(SemaExprInteger, ptr)->value = 0;
        return ptr;
    }
    case PN_EXPR_BOOL_TRUE:
    case PN_EXPR_BOOL_FALSE: {
        Index boolean = expr_new(SemaExprInteger);
        expr_(boolean)->kind = SE_INTEGER;
        expr_(boolean)->parse_node = pnode;
        expr_(boolean)->type = an.max_uint;
        expr_as(SemaExprInteger, boolean)->value = node_kind == PN_EXPR_BOOL_TRUE ? 1 : 0;
        return boolean;
    }
    case PN_EXPR_INT: {
        Token t = an.tb.at[node->main_token];
        u64 value = eval_integer(node->main_token, t.raw, t.len);
        Index integer;
        if (value > UINT32_MAX) {
            integer = expr_new(SemaExprInteger);
            expr_(integer)->kind = SE_INTEGER;
        } else {
            integer = expr_new(SemaExprInteger64);
            expr_(integer)->kind = SE_INTEGER64;
        }
        expr_(integer)->parse_node = pnode;
        expr_(integer)->type = TYPE_INT_CONSTANT; // this can cast to any integer
        if (value > UINT32_MAX) {
            expr_as(SemaExprInteger, integer)->value = (u32) value;
        } else {
            expr_as(SemaExprInteger64, integer)->value_low = (u32) value;
            expr_as(SemaExprInteger64, integer)->value_high = (u32) (value >> 32);
        }
        return integer;
    }
    case PN_EXPR_IDENT: {
        Index entity_expr = expr_new(SemaExprEntity);
        expr_(entity_expr)->kind = SE_ENTITY;
        expr_(entity_expr)->parse_node = pnode;

        Token t = an.tb.at[node->main_token];
        EntityHandle e = etable_search(tbl, t.raw, t.len);
        if (e == 0) {
            report_token(true, &an.tb, node->main_token, "'%.*s' is undefined", t.len, t.raw);
        }
        if (entity_get(e)->kind == ENTKIND_TYPENAME) {
            report_token(true, &an.tb, node->main_token, "'%.*s' is a type", t.len, t.raw);
        }
        expr_as(SemaExprEntity, entity_expr)->entity = e;
        expr_as(SemaExprEntity, entity_expr)->base.type = entity_get(e)->type;
        return entity_expr;
    }
    case PN_EXPR_DEREF: {
        Index deref = expr_new(SemaExprUnop);
        expr_(deref)->kind = SE_DEREF;
        expr_(deref)->parse_node = pnode;
        Index subexpr = sema_check_expr(tbl, node->lhs);
        expr_as(SemaExprUnop, deref)->sub = subexpr;
        TypeHandle subexpr_type = type_unalias(expr_(subexpr)->type);
        if (type_node(subexpr_type)->kind != TYPE_POINTER) {
            report_token(true, &an.tb, node->main_token, "cannot dereference %s", type_name(subexpr_type));
        }
        TypeHandle ptr_subtype = type_as(TypeNodePointer, subexpr_type)->subtype;
        if (!type_is_complete(ptr_subtype) ) {
            report_token(true, &an.tb, node->main_token, "use of incomplete type %s", type_name(ptr_subtype));
        }
        expr_(deref)->type = type_as(TypeNodePointer, subexpr_type)->subtype;
        return deref;
    };
    default:
        if (_PN_BINOP_BEGIN < node_kind && node_kind < _PN_BINOP_END) {
            Index lhs = sema_check_expr(tbl, node->lhs);
            TypeHandle lhs_type = expr_(lhs)->type;
            Index rhs = sema_check_expr(tbl, node->rhs);
            TypeHandle rhs_type = expr_(rhs)->type;

            if (!type_is_integral(lhs_type) || !type_is_integral(rhs_type)) {
                report_token(true, &an.tb, node->main_token, "operator undefined for %s and %s", type_name(lhs_type), type_name(rhs_type));
            }

            TypeHandle result_type = type_binop_result(lhs_type, rhs_type);

            Index binop = expr_new(SemaExprBinop);
            // horrendous. will probably break at some point - too bad!
            expr_(binop)->kind = node_kind + SE_ADD - PN_EXPR_ADD;
            expr_(binop)->parse_node = pnode;
            expr_(binop)->type = result_type;
            expr_as(SemaExprBinop, binop)->lhs = lhs;
            expr_as(SemaExprBinop, binop)->rhs = rhs;
            return binop;
        }

        report_token(false, &an.tb, node->main_token, "expression failed to check");
        crash("unrecognized expression kind");
    }
}

Index sema_check_var_decl(EntityTable* tbl, Index pnode_index, ParseNode* pnode, u8 kind, bool global) {
    Token ident = an.tb.at[pnode->main_token];

    u8 storage = STORAGE_LOCAL;
    if (global) switch (kind) {
    case PN_STMT_PUBLIC_DECL: storage = STORAGE_PUBLIC; break;
    case PN_STMT_DECL:
    case PN_STMT_PRIVATE_DECL: storage = STORAGE_PRIVATE; break;
    case PN_STMT_EXPORT_DECL:  storage = STORAGE_EXPORT; break;
    default:
        crash("invalid decl kind");
        break;
    }

    TypeHandle extern_type = 0;
    EntityHandle var = etable_search(tbl, ident.raw, ident.len);
    if (var != 0) { // entity found
        u8 var_kind = entity_get(var)->kind;
        if (var_kind == ENTKIND_TYPENAME) {
            report_token(true, &an.tb, pnode->main_token, "symbol is a type");
        }
        if (var_kind == ENTKIND_FN) {
            report_token(true, &an.tb, pnode->main_token, "symbol is a function");
        }
        

        u8 var_storage = entity_get(var)->storage;
        if (var_storage != STORAGE_EXTERN) {
            report_token(true, &an.tb, pnode->main_token, "symbol already declared");
        }
        if (var_storage == STORAGE_EXTERN && !global) {
            report_token(true, &an.tb, pnode->main_token, "cannot locally define an EXTERN variable");
        }


        extern_type = entity_get(var)->type;


    } else {
        var = entity_new();
        etable_put(tbl, var, ident.raw, ident.len);
    }
    entity_get(var)->storage = storage;
    entity_get(var)->is_global = global;

    Index decl = stmt_new(SemaStmtDecl);
    stmt_as(SemaStmtDecl, decl)->entity = var;
    stmt_as(SemaStmtDecl, decl)->kind = ENTKIND_VAR;
    stmt_as(SemaStmtDecl, decl)->parse_node = pnode_index;
    
    // type provided
    if (pnode->lhs != 0) {

        TypeHandle decl_type = sema_ingest_type(tbl, pnode->lhs);

        if (extern_type != 0 && !type_is_equal(extern_type, decl_type)) {
            report_token(true, &an.tb, pnode->main_token, "previous type %s does not match %s", type_name(extern_type), type_name(decl_type));
        }

        switch (type_node(decl_type)->kind) {
        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_ENUM:
        case TYPE_ENUM64:
        case TYPE_FNPTR:
        case TYPE_VARIADIC_FNPTR:
            TODO("cant decl these yet!");
        case TYPE_ALIAS_UNDEF:
            report_token(true, &an.tb, parse_node(pnode->lhs)->main_token, "use of incomplete type");
            return 0;
        case TYPE_VOID:
            report_token(true, &an.tb, parse_node(pnode->lhs)->main_token, "variable cannot be VOID");
            return 0;
        }
        entity_get(var)->type = decl_type;
        // value provided
        if (pnode->rhs != 0) {
            Index value = sema_check_expr(tbl, pnode->rhs);

            TypeHandle value_type = expr_(value)->type;
            // check that the types can implicitly cast
            bool can_implicit_cast = type_can_cast(value_type, decl_type, false);
            if (!can_implicit_cast) {
                u32 token = parse_node(expr_(value)->parse_node)->main_token;
                report_token(true, &an.tb, token, "%s cannot implicitly cast to %s", type_name(value_type), type_name(decl_type));
            }
            if (value_type != decl_type) {
                value = sema_insert_implicit_cast(value, decl_type);
            }
            
            stmt_as(SemaStmtDecl, decl)->value = value;
        }
    } else {
        // take the type from the assigned value
        Index value = sema_check_expr(tbl, pnode->rhs);

        TypeHandle value_type = expr_(value)->type;
        if (value_type == TYPE_INT_CONSTANT) {
            expr_(value)->type = an.max_int;
            value_type = an.max_int;
        }
        entity_get(var)->type = value_type;
    }
    return decl;
}

Index sema_check_extern_var_decl(EntityTable* tbl, Index pnode_index, ParseNode* pnode) {
    Token ident = an.tb.at[pnode->main_token];

    TypeHandle decl_type = sema_ingest_type(tbl, pnode->lhs);
    if (decl_type == TYPE_VOID) {
        report_token(true, &an.tb, pnode->main_token, "variable cannot store VOID");
    }

    EntityHandle var = etable_search(tbl, ident.raw, ident.len);
    if (var != 0) { // entity found
        // report_token(false, &an.tb, pnode->main_token, "entity already declared");
        u8 var_kind = entity_get(var)->kind;
        if (var_kind != ENTKIND_VAR) {
            report_token(true, &an.tb, pnode->main_token, "EXTERN symbol is not a variable");
        }
        u8 var_storage = entity_get(var)->storage;
        if (var_storage == STORAGE_PRIVATE) {
            report_token(true, &an.tb, pnode->main_token, "cannot EXTERN a PRIVATE variable");
        }
        TypeHandle var_type = entity_get(var)->type;
        if (!type_is_equal(var_type, decl_type)) {
            report_token(true, &an.tb, pnode->main_token, "previous type %s does not match %s", type_name(var_type), type_name(decl_type));
        }
    } else {
        var = entity_new();
        entity_get(var)->storage = STORAGE_EXTERN;
        entity_get(var)->type = decl_type;
        entity_get(var)->is_global = tbl == an.global;
        etable_put(tbl, var, ident.raw, ident.len);
    }

    return 0;
}

#define as_extra(T, index) ((T*)&an.pt.extra.at[index])

#define as_record(x) type_as(TypeNodeRecord, x)

void sema_check_record_decl(EntityTable* tbl, ParseNode* pnode, Index pnode_index, u8 pnode_kind) {
    u8 kind = TYPE_STRUCT;
    switch (pnode_kind) {
    case PN_STMT_UNION_DECL: kind = TYPE_UNION; break;
    case PN_STMT_STRUCT_PACKED_DECL: kind = TYPE_PACKED_STRUCT; break;
    }

    // make sure this name hasn't already been declared
    Token* name_token = &an.tb.at[pnode->lhs];
    EntityHandle ent = etable_search(tbl, name_token->raw, name_token->len);
    TypeHandle type_alias;
    if (ent != 0) {
        if (entity_get(ent)->kind != ENTKIND_TYPENAME || type_node(entity_get(ent)->type)->kind != TYPE_ALIAS_UNDEF) {
            report_token(true, &an.tb, pnode->lhs, "symbol already declared");
        }
        type_alias = entity_get(ent)->type;
    } else {
        ent = entity_new();
        entity_get(ent)->ident_string = strpool_alloc(name_token->raw, name_token->len);
        entity_get(ent)->is_global = true;
        entity_get(ent)->kind = ENTKIND_TYPENAME;
        type_alias = type_new_alias_undef(ent);
        entity_get(ent)->type = type_alias;
    }

    // parse tree references should be stationary now, since it wont ever get added to
    PNExtraList* field_list = as_extra(PNExtraList, pnode->rhs);

    TypeHandle record = type_new_record(kind, field_list->len);

    for_range(i, 0, field_list->len) {
        assert(parse_node_kind(field_list->items[i]) == PN_FIELD);
        ParseNode* pfield = parse_node(field_list->items[i]);
        
        // make sure the type is valid
        TypeHandle t = sema_ingest_type(tbl, pfield->rhs);
        if (!type_is_complete(t)) {
            report_token(true, &an.tb, pfield->main_token + 2, "use of incomplete type");
        }

        Token* field_token = &an.tb.at[pfield->lhs];

        // make sure the name isn't already present
        for_range(f, 0, i) {
            if (strpool_eq(as_record(record)->fields->name, field_token->raw, field_token->len)) {
                report_token(true, &an.tb, pfield->main_token, "duplicate field name");
            }
        }

        usize field_size = type_size(t);
        usize field_align = type_align(t);
        usize field_offset = 0;
        usize new_size = 0;
        usize new_align = 1;

        switch (kind) {
        case TYPE_STRUCT:
            field_offset = align_forward(as_record(record)->size, field_align);
            new_size = align_forward(field_offset + field_size, new_align);
            new_align = max(as_record(record)->align, field_align);
            break;
        case TYPE_UNION:
            field_offset = 0;
            new_size = max(as_record(record)->size, field_size);
            new_align = max(as_record(record)->align, field_align);
            break;
        case TYPE_PACKED_STRUCT:
            field_offset = as_record(record)->size;
            new_size = as_record(record)->size + field_size;
            break;
        }

        // add field
        as_record(record)->fields[i].name = strpool_alloc(field_token->raw, field_token->len);
        as_record(record)->fields[i].type = t;
        as_record(record)->fields[i].offset = field_offset;
        as_record(record)->size = new_size;
        as_record(record)->align = new_align;
    }

    // create alias
    type_node(type_alias)->kind = TYPE_ALIAS;
    type_as(TypeNodeAlias, type_alias)->subtype = record;

    // TODO("record decl");
}

Index sema_check_stmt(EntityTable* tbl, Index pnode) {

    bool is_global = tbl == an.global;

    ParseNode* node = parse_node(pnode);
    u8 node_kind = parse_node_kind(pnode);

    switch (node_kind) {
        
    case PN_STMT_PUBLIC_DECL:
    case PN_STMT_EXPORT_DECL:
    case PN_STMT_PRIVATE_DECL:
        if (!is_global) {
            report_token(true, &an.tb, node->main_token - 1, "qualified declaration must be global");
        }
    case PN_STMT_DECL:
        return sema_check_var_decl(tbl, pnode, node, node_kind, is_global);
    case PN_STMT_EXTERN_DECL:
        sema_check_extern_var_decl(tbl, pnode, node);
        return 0; // extern decls dont get added to the sema tree
    case PN_STMT_STRUCT_DECL:
    case PN_STMT_STRUCT_PACKED_DECL:
    case PN_STMT_UNION_DECL:
        if (!is_global) {
            report_token(true, &an.tb, node->main_token, "type declaration must be global");
        }
        sema_check_record_decl(tbl, node, pnode, node_kind);
        return 0;
    default:
        report_token(true, &an.tb, node->main_token, "unknown decl");
        return 0;
    }
}

Analyzer sema_analyze(ParseTree pt, TokenBuf tb) {
    sema_init();
    an.pt = pt;
    an.tb = tb;

    for_range(i, 0, pt.len) {
        Index pdecl = pt.decls[i];
        Index sdecl = sema_check_stmt(an.global, pdecl);
    }

    return an;
}