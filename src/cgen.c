#include "cgen.h"

static Analyzer an;
static StringBuilder* sb;

void cgen_emit_prelude() {
    sb_append_c(sb, "#include <stdint.h>\n");
    sb_append_c(sb, "typedef int8_t   BYTE;\n");
    sb_append_c(sb, "typedef uint8_t  UBYTE;\n");
    sb_append_c(sb, "typedef int16_t  INT;\n");
    sb_append_c(sb, "typedef uint16_t UINT;\n");
    sb_append_c(sb, "typedef int32_t  LONG;\n");
    sb_append_c(sb, "typedef uint32_t ULONG;\n");
    sb_append_c(sb, "typedef int64_t  QUAD;\n");
    sb_append_c(sb, "typedef uint64_t UQUAD;\n");
    sb_append_c(sb, "\n");
}

static char* entity_name(EntityHandle e) {
    Entity* ent = entity_get(e);
    char* ident = &an.strings.chars[ent->ident_string];
    return ident;
}

static void emit_structure_typedecl(TypeHandle t) {
    sb_printf(sb, "{}");
}

void cgen_ensure_typedecl(TypeHandle t, const char* ident) {
    if (t < _TYPE_SIMPLE_END) return;
    if (type_node(t)->emitted) return; // dont re-emit already emitted types

    switch (type_node(t)->kind) {
    case TYPE_ALIAS:
        const char* alias_ident = entity_name(type_as(TypeNodeAlias, t)->entity);
        cgen_ensure_typedecl(type_as(TypeNodeAlias, t)->subtype, alias_ident);

        sb_printf(sb, "typedef %s %s;", alias_ident, ident);
        break;
    case TYPE_STRUCT:
        sb_printf(sb, "typedef %s %s;", alias_ident, ident);
    }

    if (type_node(t)->emitted) return; // dont re-emit already emitted types
}

void cgen_emit_all_typedecls() {
    // unknown types just get emitted as anon structs
    for_range(i, 0, an.global->cap) {
        EntityHandle ent_handle = an.global->entities[i];
        if (ent_handle == 0) continue;
        Entity* ent = entity_get(ent_handle);
        if (ent->kind != ENTKIND_TYPENAME) continue;

        if (type_node(ent->type)->kind == TYPE_ALIAS_UNDEF) {
            sb_printf(sb, "typedef struct %s %s;", entity_name(ent_handle), entity_name(ent_handle));
        }
    }

    for_range(i, 0, an.global->cap) {
        EntityHandle ent_handle = an.global->entities[i];
        if (ent_handle == 0) continue;
        Entity* ent = entity_get(ent_handle);
        if (ent->kind != ENTKIND_TYPENAME) continue;

        if (type_node(ent->type)->kind == TYPE_ALIAS_UNDEF) continue;

        const char* name = entity_name(ent_handle);
        cgen_ensure_typedecl(type_as(TypeNodeAlias, ent->type)->subtype, name);
        
    }
}

string cgen_emit_all(Analyzer analyzer) {
    an = analyzer;

    StringBuilder sbuilder;
    sb = &sbuilder;
    sb_init(sb);


    cgen_emit_prelude();
    cgen_emit_all_typedecls();

    string out;
    out.len = sb->len;
    out.raw = malloc(sb->len);
    sb_write(sb, out.raw);

    sb_destroy(sb);
    return out;
}