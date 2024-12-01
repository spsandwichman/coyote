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

void cgen_emit_typedecl(TypeHandle t, char* ident) {
    if (t < _TYPE_SIMPLE_END) return;
    // if (type_node(t)->emitted) return; // dont re-emit already emitted types

    switch (type_node(t)->kind) {
    case TYPE_ALIAS:
        char* alias_ident = entity_name(type_as(TypeNodeAlias, t)->entity);
        cgen_emit_typedecl(type_as(TypeNodeAlias, t)->subtype, alias_ident);
        // sb_printf(sb, "typedef %s %s;\n", alias_ident, ident);
        break;
    case TYPE_STRUCT:
        if (type_node(t)->visited) {
            sb_printf(sb, "typedef struct %s %s;\n", ident, ident);
        } else {
            type_node(t)->visited = true;
            
            // check that all dependencies have been emitted
            for_range(i, 0, type_as(TypeNodeRecord, t)->len) {
                cgen_emit_typedecl(type_as(TypeNodeRecord, t)->fields[i].type, "$$");
            }
            
            sb_printf(sb, "typedef struct %s ", ident);
            emit_structure_typedecl(t);
            sb_printf(sb, " %s;\n", ident);
        }
        break;
    default:
        // crash("unhandled typedecl kind");
    }
    type_node(t)->emitted = true;
    type_node(t)->visited = false;
}

void cgen_emit_all_typedecls() {
    for_range(i, 0, an.global->cap) {
        if (an.global->entities[i] == 0) continue;
        Entity* ent = entity_get(an.global->entities[i]);
        if (ent->kind != ENTKIND_TYPENAME) continue;

        cgen_emit_typedecl(ent->type, "");
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