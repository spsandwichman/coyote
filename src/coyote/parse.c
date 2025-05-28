#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "parse.h"
#include "common/str.h"
#include "common/strmap.h"
#include "common/util.h"
#include "common/vec.h"
#include "coyote.h"
#include "lex.h"


Vec_typedef(char);

static void vec_char_append_str(Vec(char)* vec, const char* data) {
    usize len = strlen(data);
    for_n(i, 0, len) {
        vec_append(vec, data[i]);
    }
}

static void vec_char_append_many(Vec(char)* vec, const char* data, usize len) {
    for_n(i, 0, len) {
        vec_append(vec, data[i]);
    }
}

thread_local static struct {
    TyBufSlot* at;
    TyIndex* ptrs;
    u32 len;
    u32 cap;
} tybuf;

#define TY(index, T) ((T*)&tybuf.at[index])
#define TY_KIND(index) ((TyBase*)&tybuf.at[index])->kind

void ty_init() {
    tybuf.len = 0;
    tybuf.cap = 1024;
    tybuf.at = malloc(sizeof(tybuf.at[0]) * tybuf.cap);
    tybuf.ptrs = malloc(sizeof(tybuf.ptrs[0]) * tybuf.cap);
    memset(tybuf.ptrs, 0, sizeof(tybuf.ptrs[0]) * tybuf.cap);
    
    for_n_eq(i, TY_VOID, TY_UQUAD) {
        TY(i,  TyBase)->kind = i;

        // init pointer cache
        TY(i + TY_PTR,  TyPtr)->kind = TY_PTR;
        TY(i + TY_PTR,  TyPtr)->to = i;
        tybuf.ptrs[i] = i + TY_PTR;
    }
    tybuf.len = TY_UQUAD + TY_PTR + 1;
}

#define ty_allocate(T) ty__allocate(sizeof(T), alignof(T) == 8)
static TyIndex ty__allocate(usize size, bool align64) {
    usize old_len = tybuf.len;

    usize slots = size / sizeof(TyBufSlot);
    if (align64 && (tybuf.len & 1)) {
        tybuf.len += 1; // pad to 64
    }
    
    if (tybuf.len + slots > tybuf.cap) {
        tybuf.cap += tybuf.cap << 1;
        tybuf.at = realloc(tybuf.at, sizeof(tybuf.at[0]) * tybuf.cap);
        tybuf.ptrs = realloc(tybuf.ptrs, sizeof(tybuf.ptrs[0]) * tybuf.cap);
        memset(&tybuf.ptrs[old_len], 0, sizeof(tybuf.ptrs[0]) * (tybuf.cap - old_len));
    }

    usize pos = tybuf.len;
    tybuf.len += slots;
    return pos;
}

static TyIndex ty_get_ptr_target(TyIndex ptr) {
    return TY(ptr, TyPtr)->to;
}

static TyIndex ty_get_ptr(TyIndex t) {
    if (t < TY_PTR) {
        return t + TY_PTR;
    }
    if (tybuf.ptrs[t] == 0) {
        // we have to create a pointer type since none exists.
        TyIndex ptr = ty_allocate(TyPtr);
        TY(ptr, TyPtr)->kind = TY_PTR;
        TY(ptr, TyPtr)->to = t;
        
        tybuf.ptrs[t] = ptr;
        return ptr;
    } else {
        return tybuf.ptrs[t];
    }
}


static void _ty_name(Vec(char)* v, TyIndex t) {
    switch (TY_KIND(t)) {
    case TY__INVALID: vec_char_append_str(v, "INVALID"); return;
    case TY_VOID:     vec_char_append_str(v, "VOID"); return;
    case TY_BYTE:     vec_char_append_str(v, "BYTE"); return;
    case TY_UBYTE:    vec_char_append_str(v, "UBYTE"); return;
    case TY_INT:      vec_char_append_str(v, "INT"); return;
    case TY_UINT:     vec_char_append_str(v, "UINT"); return;
    case TY_LONG:     vec_char_append_str(v, "LONG"); return;
    case TY_ULONG:    vec_char_append_str(v, "ULONG"); return;
    case TY_QUAD:     vec_char_append_str(v, "QUAD"); return;
    case TY_UQUAD:    vec_char_append_str(v, "UQUAD"); return;
    case TY_PTR:
        vec_char_append_str(v, "^");
        _ty_name(v, ty_get_ptr_target(t));
        return;
    default: vec_char_append_str(v, "???");
    }
}

const char* ty_name(TyIndex t) {
    Vec(char) buf = vec_new(char, 16);
    _ty_name(&buf, t);
    vec_append(&buf, 0);
    return buf.at;
}

static bool ty_is_scalar(TyIndex t) {
    if (t == TY_VOID) {
        return false;
    }
    if (t < TY_PTR) {
        return true;
    }
    u8 ty_kind = TY(t, TyBase)->kind;
    if (ty_kind == TY_PTR || ty_kind == TY_FNPTR) {
        return true;
    }
    return false;
}

static bool ty_is_integer(TyIndex t) {
    if (t == TY_VOID) {
        return false;
    }
    if (t < TY_PTR) {
        return true;
    }
    return false;
}

static bool ty_is_signed(TyIndex t) {
    switch (t) {
    case TY_BYTE:
    case TY_INT:
    case TY_LONG:
    case TY_QUAD:
        return true;
    default:
        return false;
    }
}

thread_local static usize target_ptr_size = 4;
thread_local static TyIndex target_word  = TY_LONG;
thread_local static TyIndex target_uword = TY_ULONG;
#define TY_VOIDPTR (TY_VOID + TY_PTR)

static usize ty_size(TyIndex t) {
    switch (t) {
    case TY_VOID: return 0;
    case TY_BYTE:
    case TY_UBYTE: return 1;
    case TY_INT:
    case TY_UINT: return 2;
    case TY_LONG:
    case TY_ULONG: return 4;
    case TY_QUAD:
    case TY_UQUAD: return 8;
    }
    switch (TY_KIND(t)) {
    case TY_PTR:
        return target_ptr_size;
    case TY_ARRAY:
        return TY(t, TyArray)->len * ty_size(TY(t, TyArray)->to);
    default:
        return 0;
    }
    TODO("AAAA");
}

static bool ty_equal(TyIndex t1, TyIndex t2) {
    if (t1 == t2) {
        return true;
    }
    while (TY_KIND(t1) == TY_ALIAS) {
        t1 = TY(t1, TyAlias)->aliasing;
    }
    while (TY_KIND(t2) == TY_ALIAS) {
        t2 = TY(t2, TyAlias)->aliasing;
    }
    return t1 == t2;
}

static bool ty_compatible(TyIndex dst, TyIndex src, bool src_is_constant) {
    if (dst == src) {
        return true;
    }
    while (TY_KIND(dst) == TY_ALIAS) {
        dst = TY(dst, TyAlias)->aliasing;
    }
    while (TY_KIND(src) == TY_ALIAS) {
        src = TY(src, TyAlias)->aliasing;
    }
    if (dst == src) {
        return true;
    }

    if (ty_is_integer(dst) && ty_is_integer(src)) {
        // signedness cannot mix
        return src_is_constant || ty_is_signed(dst) == ty_is_signed(src);
    }

    if (TY_KIND(dst) == TY_PTR && (src == TY_VOIDPTR || src_is_constant)) {
        return true;
    }
    if (TY_KIND(src) == TY_PTR && (dst == TY_VOIDPTR || dst == target_uword)) {
        return true;
    }

    return false;
}

void enter_scope(Parser* p) {
    // re-use subscope if possible
    if (p->current_scope->sub) {
        p->current_scope = p->current_scope->sub;
        if (p->current_scope->map.size != 0) {
            strmap_reset(&p->current_scope->map);
        }
        return;
    }

    // create a new scope
    ParseScope* scope = malloc(sizeof(ParseScope));
    scope->sub = nullptr;
    scope->super = p->current_scope;
    p->current_scope->sub = scope;
    strmap_init(&scope->map, 128);

    p->current_scope = p->current_scope->sub;
}

void exit_scope(Parser* p) {
    if (p->current_scope == p->global_scope) {
        CRASH("exited global scope");
    }
    p->current_scope = p->current_scope->super;
}

// general dynamic buffer for parsing shit

VecPtr_typedef(void);
static thread_local VecPtr(void) dynbuf;

static inline usize dynbuf_start() {
    return dynbuf.len;
}

static inline void dynbuf_restore(usize start) {
    dynbuf.len = start;
}

static inline usize dynbuf_len(usize start) {
    return dynbuf.len - start;
}

static inline void** dynbuf_to_arena(Parser* p, usize start) {
    usize len = dynbuf.len - start;
    void** items = arena_alloc(&p->arena, len * sizeof(items[0]), alignof(void*));
    memcpy(items, &dynbuf.at[len], len * sizeof(items[0]));
    return items;
}

Entity* new_entity(Parser* p, string ident, EntityKind kind) {
    Entity* entity = arena_alloc(&p->arena, sizeof(Entity), alignof(Entity));
    entity->name = to_compact(ident);
    entity->kind = kind;
    entity->ty = TY__INVALID;
    strmap_put(&p->current_scope->map, ident, entity);
    return entity;
}

Entity* get_entity(Parser* p, string key) {
    ParseScope* scope = p->current_scope;
    while (scope) {
        Entity* entity = strmap_get(&scope->map, key);
        if (entity != STRMAP_NOT_FOUND) {
            return entity;
        }
        scope = scope->super;
    }
    return nullptr;
}

static bool token_is_within(SrcFile* f, char* raw) {
    return (uintptr_t)f->src.raw < (uintptr_t)raw
        && (uintptr_t)f->src.raw + f->src.len > (uintptr_t)raw;
}

static SrcFile* where_from(Parser* ctx, string span) {
    for_n (i, 0, ctx->sources.len) {
        SrcFile* f = ctx->sources.at[i];
        if (token_is_within(f, span.raw)) {
            return f;
        }
    }
    return nullptr;
}

static usize preproc_depth(Parser* ctx, u32 index) {
    usize depth = 0;
    for_n(i, 0, index) {
        Token t = ctx->tokens[i];
        switch (t.kind) {
        case TOK_PREPROC_MACRO_PASTE:
        case TOK_PREPROC_DEFINE_PASTE:
        case TOK_PREPROC_INCLUDE_PASTE:
            depth++;
            break;
        case TOK_PREPROC_PASTE_END:
            depth--;
            break;
        }
    }
    return depth;
}

void token_error(Parser* ctx, ReportKind kind, u32 start_index, u32 end_index, const char* msg) {
    // find out if we're in a macro somewhere
    bool inside_preproc = preproc_depth(ctx, start_index) != 0 || preproc_depth(ctx, end_index) != 0;

    Vec_typedef(ReportLine);
    Vec(ReportLine) reports = vec_new(ReportLine, 8);

    i32 unmatched_ends = 0;
    for (i64 i = (i64)start_index; i > 0; --i) {
        Token t = ctx->tokens[i];

        ReportLine report = {};
        report.kind = REPORT_NOTE;

        switch (t.kind) {
        case TOK_PREPROC_DEFINE_PASTE:
        case TOK_PREPROC_MACRO_PASTE:
            if (unmatched_ends != 0) {
                unmatched_ends--;
                break;
            }
            SrcFile* from = where_from(ctx, tok_span(t));
            if (!from) {
                CRASH("unable to locate macro paste span source file");
            }
            report.msg = strprintf("using macro '"str_fmt"'", str_arg(tok_span(t)));
            report.path = from->path;
            report.src = from->src;
            report.snippet = tok_span(t);
            vec_append(&reports, report);
            break;
        case TOK_PREPROC_PASTE_END:
            unmatched_ends++;
            break;
        }
    }

    
    // find main line snippet
    SrcFile* main_file = ctx->sources.at[0];
    if (inside_preproc) {
        for_n(i, 0, reports.len) {
            report_line(&reports.at[i]);
        }

        string main_highlight = {};
        for_n(i, end_index, ctx->tokens_len) {
            Token t = ctx->tokens[i];
            if (t.kind == TOK_PREPROC_PASTE_END && preproc_depth(ctx, i + 1) == 0) {
                main_highlight = tok_span(t);
                break;
            }
        }

        // vec_append(&reports, main_line_report);


        // construct the line
        u32 expanded_snippet_begin_index = start_index;    
        while (true) {
            u8 kind = ctx->tokens[expanded_snippet_begin_index].kind;
            if (kind == TOK_NEWLINE) {
                expanded_snippet_begin_index += 1;
                break;
            }
            if (expanded_snippet_begin_index == 0) {
                break;
            }
            expanded_snippet_begin_index -= 1;
        }
        u32 expanded_snippet_end_index = end_index;
        while (true) {
            u8 kind = ctx->tokens[expanded_snippet_end_index].kind;
            if (kind == TOK_NEWLINE || expanded_snippet_end_index == ctx->tokens_len - 1) {
                break;
            }
            expanded_snippet_end_index += 1;
        }

        u32 expanded_snippet_highlight_start = 0;
        u32 expanded_snippet_highlight_len = 0;

        Vec(char) expanded_snippet = vec_new(char, 256);

        for_n_eq(i, expanded_snippet_begin_index, expanded_snippet_end_index) {
            if (i == start_index) {
                expanded_snippet_highlight_start = expanded_snippet.len;
            }
            Token t = ctx->tokens[i];
            if (_TOK_LEX_IGNORE < t.kind) {
                continue;
            }
            vec_char_append_many(&expanded_snippet, tok_raw(t), t.len);
            if (i == end_index) {
                expanded_snippet_highlight_len = expanded_snippet.len - expanded_snippet_highlight_start;
            }
            vec_append(&expanded_snippet, ' ');
        }

        string src = {
            .raw = expanded_snippet.at,
            .len = expanded_snippet.len,
        };
        string snippet = {
            .raw = expanded_snippet.at + expanded_snippet_highlight_start,
            .len = expanded_snippet_highlight_len,
        };

        ReportLine rep = {
            .kind = kind,
            .msg = str(msg),
            .path = main_file->path,
            .src = main_file->src,
            .snippet = main_highlight,

            .reconstructed_line = src,
            .reconstructed_snippet = snippet,
        };

        report_line(&rep);
    } else {
        string start_span = tok_span(ctx->tokens[start_index]);
        string end_span = tok_span(ctx->tokens[end_index]);
        string span = {
            .raw = start_span.raw,
            .len = (usize)end_span.raw - (usize)start_span.raw + end_span.len,
        };
        ReportLine rep = {
            .kind = kind,
            .msg = str(msg),
            .path = main_file->path,
            .snippet = span,
            .src = main_file->src,
        };

        report_line(&rep);
    }
}

static void advance(Parser* p) {
    do { // skip past "transparent" tokens.
        if (p->tokens[p->cursor].kind == TOK_EOF) {
            break;
        }
        ++p->cursor;
        switch (p->tokens[p->cursor].kind) {
        case TOK_PREPROC_DEFINE_PASTE:
        case TOK_PREPROC_MACRO_PASTE:
            enter_scope(p);
            break;
        case TOK_PREPROC_PASTE_END:
            exit_scope(p);
            break;
        }
    } while (_TOK_LEX_IGNORE < p->tokens[p->cursor].kind);
    p->current = p->tokens[p->cursor];
}

static Token peek(Parser* p, usize n) {
    u32 cursor = p->cursor;
    for_n(_, 0, n) {
        do { // skip past "transparent" tokens.
            if (p->tokens[cursor].kind == TOK_EOF) {
                return p->tokens[cursor];
            }
            ++cursor;
        } while (_TOK_LEX_IGNORE < p->tokens[cursor].kind);
    }
    return p->tokens[cursor];
}

static bool match(Parser* p, u8 kind) {
    return p->current.kind == kind;
}

static void parse_error(Parser* p, u32 start, u32 end, ReportKind kind, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    thread_local static char sprintf_buf[512];

    vsnprintf(sprintf_buf, sizeof(sprintf_buf), fmt, args);

    va_end(args);

    token_error(p, kind, start, end, sprintf_buf);
}

static void expect(Parser* p, u8 kind) {
    if (kind != p->current.kind) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "expected '%s', got '%s'", token_kind[kind], token_kind[p->current.kind]);
    }
}

static void expect_advance(Parser* p, u8 kind) {
    expect(p, kind);
    advance(p);
}

#define new_expr(p, kind, ty, field) \
    new_expr_(p, kind, ty, offsetof(Expr, extra) + sizeof(((Expr*)nullptr)->field))

static Expr* new_expr_(Parser* p, u8 kind, TyIndex ty, usize size) {
    Expr* expr = arena_alloc(&p->arena, size, alignof(Expr));
    expr->kind = kind;
    expr->ty = ty;
    expr->token_index = p->cursor;
    return expr;
}

static i64 eval_integer(Parser* p, Token t, u32 index) {
    i64 val = 0;
    char* raw = tok_raw(t);
    bool is_negative = raw[0] == '-';

    for (usize i = is_negative; i < t.len; ++i) {
        val *= 10;
        isize char_val = raw[i] - '0';
        if (char_val < 0 || char_val > 9) {
            // TODO("invalid digit");
            parse_error(p, index, index, REPORT_ERROR, "invalid digit '%c'", raw[i]);
        }
        val += raw[i] - '0';
    }

    return is_negative ? -val : val;
}

u32 expr_leftmost_token(Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
    case EXPR_STR_LITERAL:
    case EXPR_NOT:
        return expr->token_index;
    case EXPR_DEREF:
        return expr_leftmost_token(expr->unary);
    default:
        TODO("unknown expr kind");
    }
}

u32 expr_rightmost_token(Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
    case EXPR_STR_LITERAL:
    case EXPR_DEREF:
        return expr->token_index;
    case EXPR_NOT:
        return expr_rightmost_token(expr->unary);
    default:
        TODO("unknown expr kind");
    }
}

#define error_at_expr(p, expr, report_kind, msg, ...) \
    parse_error(p, expr_leftmost_token(expr), expr_rightmost_token(expr), report_kind, msg __VA_OPT__(,) __VA_ARGS__)

TyIndex parse_type_terminal(Parser* p, bool allow_incomplete) {
    switch (p->current.kind) {
    case TOK_KW_VOID:  advance(p); return TY_VOID;
    case TOK_KW_BYTE:  advance(p); return TY_BYTE;
    case TOK_KW_UBYTE: advance(p); return TY_UBYTE;
    case TOK_KW_INT:   advance(p); return TY_INT;
    case TOK_KW_UINT:  advance(p); return TY_UINT;
    case TOK_KW_LONG:  advance(p); return TY_LONG;
    case TOK_KW_ULONG: advance(p); return TY_ULONG;
    case TOK_KW_QUAD:  advance(p); return TY_QUAD;
    case TOK_KW_UQUAD: advance(p); return TY_UQUAD;
    case TOK_KW_WORD:  advance(p); return target_word;
    case TOK_KW_UWORD: advance(p); return target_uword;
    case TOK_OPEN_PAREN:
        advance(p);
        TyIndex inner = parse_type(p, allow_incomplete);
        expect_advance(p, TOK_CLOSE_PAREN);
        return inner;
    case TOK_CARET:
        advance(p);
        return ty_get_ptr(parse_type_terminal(p, true));
    case TOK_IDENTIFIER:
        ;
        string span = tok_span(p->current);
        Entity* entity = get_entity(p, span);
        if (!entity) { // create an incomplete type
            if (!allow_incomplete) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot use incomplete type");
            }
            TyIndex incomplete = ty_allocate(TyAlias);
            entity = new_entity(p, span, ENTKIND_TYPE);
            entity->ty = incomplete;
            advance(p);
            return incomplete;
        }
        if (entity->kind != ENTKIND_TYPE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "symbol is not a type");
        }
        advance(p);
        TyIndex t = entity->ty;
        while (TY_KIND(t) == TY_ALIAS) {
            t = TY(t, TyAlias)->aliasing;
        }
        return t;
    default:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "expected type expression");
    }
    return TY_VOID;
}

TyIndex parse_type(Parser* p, bool allow_incomplete) {
    TyIndex left = parse_type_terminal(p, allow_incomplete);
    while (p->current.kind == TOK_OPEN_BRACKET) {
        if (TY_KIND(left) == TY_ALIAS_INCOMPLETE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot use incomplete type");
        }
        advance(p);
        TyIndex arr = ty_allocate(TyArray);
        TY(arr, TyArray)->to = left;
        TY(arr, TyArray)->kind = TY_ARRAY;
        Expr* len_expr = parse_expr(p);

        if (!ty_is_integer(len_expr->ty)) {
            error_at_expr(p, len_expr, REPORT_ERROR, "array length must be integer");
        }
        if (len_expr->kind != EXPR_LITERAL) {
            error_at_expr(p, len_expr, REPORT_ERROR, "array length must be compile-time known");
        }
        if (len_expr->literal > (u64)INT32_MAX) {
            error_at_expr(p, len_expr, REPORT_WARNING, "array length is... excessive");
        }
        TY(arr, TyArray)->len = len_expr->literal;
        expect_advance(p, TOK_CLOSE_BRACKET);
        left = arr;
    }
    return left;
}

Expr* parse_atom_terminal(Parser* p) {
    string span = tok_span(p->current);
    Expr* atom = nullptr;
    switch (p->current.kind) {
    case TOK_OPEN_PAREN:
        advance(p);
        atom = parse_expr(p);
        expect_advance(p, TOK_CLOSE_PAREN);
        break;
    case TOK_INTEGER:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        atom->literal = eval_integer(p, p->current, p->cursor);
        advance(p);
        break;
    case TOK_KW_TRUE:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        atom->literal = 1;
        advance(p);
        break;
    case TOK_KW_FALSE:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        atom->literal = 0;
        advance(p);
        break;
    case TOK_KW_NULLPTR:
        atom = new_expr(p, EXPR_LITERAL, ty_get_ptr(TY_VOID), literal);
        atom->literal = 0;
        advance(p);
        break;
    case TOK_KW_SIZEOF:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        advance(p);
        TyIndex type = parse_type(p, false); 
        atom->literal = ty_size(type);
        break;
    case TOK_IDENTIFIER:
        // find an entity
        atom = new_expr(p, EXPR_ENTITY, TY__INVALID, entity);
        Entity* entity = get_entity(p, span);
        if (entity == nullptr) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
                "symbol does not exist");
        }
        if (entity->kind == ENTKIND_TYPE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
                "entity is a type");
        }
        atom->entity = entity;
        atom->ty = entity->ty;
        advance(p);
        break;
    default:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "expected expression");
    }

    return atom;
}

Expr* parse_atom(Parser* p) {
    Expr* atom = parse_atom_terminal(p);
    Expr* left;

    switch (p->current.kind) {
    case TOK_CARET:
        if (peek(p, 1).kind == TOK_DOT) {
            advance(p);
            goto member_deref;
        }
        left = atom;
        if (TY_KIND(left->ty) != TY_PTR) {
            parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
                "cannot dereference type %s", ty_name(left->ty));
        }
        TyIndex ptr_target = ty_get_ptr_target(left->ty);
        if (!ty_is_scalar(ptr_target)) {
            parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
                "cannot use non-scalar type %s", ty_name(ptr_target));
        }
        atom = new_expr(p, EXPR_DEREF, ptr_target, unary);
        atom->unary = left;
        break;
    case TOK_CARET_DOT:
        member_deref:
        UNREACHABLE;
        break;
    default:
        break;
    }

    return atom;
}

Expr* parse_unary(Parser* p) {
    ArenaState save = arena_save(&p->arena);

    u32 op_position = p->cursor;

    switch (p->current.kind) {
    case TOK_KW_NOT:
        advance(p);
        Expr* inner = parse_unary(p); 
        if (inner->kind == EXPR_LITERAL) {
            inner->literal = !inner->literal; // reuse this expr
            return inner;
        } else {
            Expr* not = new_expr(p, EXPR_BOOL_NOT, inner->ty, unary);
            not->unary = inner;
            not->token_index = op_position;
            return not;
        }
    default:
        return parse_atom(p);
    }
}

static isize bin_precedence(u8 kind) {
    switch (kind) {
        case TOK_MUL:
        case TOK_DIV:
        case TOK_MOD:
            return 10;
        case TOK_PLUS:
        case TOK_MINUS:
            return 9;
        case TOK_LSHIFT:
        case TOK_RSHIFT:
            return 8;
        case TOK_AND:
            return 7;
        case TOK_XOR:
            return 6;
        case TOK_OR:
            return 5;
        case TOK_LESS:
        case TOK_LESS_EQ:
        case TOK_GREATER:
        case TOK_GREATER_EQ:
            return 4;
        case TOK_EQ_EQ:
        case TOK_NOT_EQ:
            return 3;
        case TOK_KW_AND:
            return 2;
        case TOK_KW_OR:
            return 1;
    }
    return -1;
}

static ExprKind binary_expr_kind(u8 tok_kind) {
    return tok_kind - TOK_PLUS + EXPR_ADD;
}

Expr* parse_binary(Parser* p, isize precedence) {
    ArenaState save = arena_save(&p->arena);
    Expr* lhs = parse_unary(p);
    
    while (precedence < bin_precedence(p->current.kind)) {
        isize n_prec = bin_precedence(p->current.kind);
        ExprKind op_kind = binary_expr_kind(p->current.kind);
        u32 op_token = p->cursor;
        
        advance(p);
        Expr* rhs = parse_binary(p, n_prec);

        if (!ty_compatible(lhs->ty, rhs->ty, rhs->kind == EXPR_LITERAL)) {
            parse_error(p, op_token, op_token, REPORT_ERROR, 
                "type %u and %u are not compatible", ty_name(lhs->ty), ty_name(rhs->ty));
        }

        TyIndex op_ty = lhs->ty;
        if (lhs->kind == EXPR_LITERAL && rhs->ty != EXPR_LITERAL) {
            op_ty = rhs->ty;
        }
        
        if (lhs->kind == EXPR_LITERAL && rhs->kind == EXPR_LITERAL) {
            bool signed_op = ty_is_signed(lhs->ty) || ty_is_signed(rhs->ty);
            u64 lhs_val = lhs->literal;
            u64 rhs_val = rhs->literal;
            u32 leftmost = expr_leftmost_token(lhs);

            arena_restore(&p->arena, save);
            Expr* lit = new_expr(p, EXPR_LITERAL, op_ty, literal);
            lit->token_index = leftmost;
            
            switch (op_kind) {
            case EXPR_ADD: lit->literal = lhs_val + rhs_val; break;
            case EXPR_SUB: lit->literal = lhs_val - rhs_val; break;
            case EXPR_MUL: lit->literal = lhs_val * rhs_val; break;
            case EXPR_DIV: 
                if (signed_op) {
                    lit->literal = (isize)lhs_val / (isize)rhs_val;
                } else {
                    lit->literal = lhs_val / rhs_val;
                }
                break;
            case EXPR_MOD: 
                if (signed_op) {
                    lit->literal = (isize)lhs_val % (isize)rhs_val;
                } else {
                    lit->literal = lhs_val % rhs_val;
                }
                break;
            case EXPR_AND: lit->literal = lhs_val & rhs_val; break;
            case EXPR_OR:  lit->literal = lhs_val | rhs_val; break;
            case EXPR_XOR: lit->literal = lhs_val ^ rhs_val; break;
            case EXPR_LSH: lit->literal = lhs_val << rhs_val; break;
            case EXPR_RSH:
                if (signed_op) {
                    lit->literal = (isize)lhs_val >> (isize)rhs_val;
                } else {
                    lit->literal = lhs_val >> rhs_val;
                }
                break;
            case EXPR_EQ:  lit->literal = lhs_val == rhs_val; break;
            case EXPR_NEQ: lit->literal = lhs_val != rhs_val; break;
            case EXPR_LESS_EQ:
                if (signed_op) {
                    lit->literal = (isize)lhs_val <= (isize)rhs_val;
                } else {
                    lit->literal = lhs_val <= rhs_val;
                }
                break;
            case EXPR_GREATER_EQ:
                if (signed_op) {
                    lit->literal = (isize)lhs_val >= (isize)rhs_val;
                } else {
                    lit->literal = lhs_val >= rhs_val;
                }
                break;
            case EXPR_LESS:
                if (signed_op) {
                    lit->literal = (isize)lhs_val < (isize)rhs_val;
                } else {
                    lit->literal = lhs_val < rhs_val;
                }
                break;
            case EXPR_GREATER:
                if (signed_op) {
                    lit->literal = (isize)lhs_val > (isize)rhs_val;
                } else {
                    lit->literal = lhs_val > rhs_val;
                }
                break;
            default:
                UNREACHABLE;
            }
            // printf("consteval %lld\n", lit->literal);
            lhs = lit;
        } else {
            Expr* op = new_expr(p, op_kind, op_ty, binary); 
            op->binary.lhs = lhs;
            op->binary.rhs = rhs;
            lhs = op;
        }
    }

    return lhs;
}

Expr* parse_expr(Parser* p) {
    return parse_binary(p, 0);
}

#define new_stmt(p, kind, field) \
    new_stmt_(p, kind, offsetof(Stmt, extra) + sizeof(((Stmt*)nullptr)->field))

static Stmt* new_stmt_(Parser* p, u8 kind, usize size) {
    Stmt* stmt = arena_alloc(&p->arena, size, alignof(Stmt));
    stmt->kind = kind;
    stmt->token_index = p->cursor;
    return stmt;
}

static Entity* get_or_create(Parser* p, string ident) {
    Entity* entity = get_entity(p, ident);
    if (!entity) {
        entity = new_entity(p, ident, ENTKIND_VAR);
    } else if (entity->storage != STORAGE_EXTERN) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "symbol already exists");
    }
    return entity;
}

Stmt* parse_var_decl(Parser* p, StorageKind storage) {
    Stmt* decl = new_stmt(p, STMT_VAR_DECL, var_decl);
    
    string identifier = tok_span(p->current);
    Entity* var = get_or_create(p, identifier);
    decl->var_decl.var = var;
    if (var->storage == STORAGE_EXTERN && storage == STORAGE_PRIVATE) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "previously EXTERN variable cannot be PRIVATE");
    }
    advance(p);
    expect_advance(p, TOK_COLON);

    if (!match(p, TOK_EQ)) {
        u32 type_start = p->cursor;
        TyIndex decl_ty = parse_type(p, false);
        if (var->storage == STORAGE_EXTERN && !ty_equal(var->ty, decl_ty)) {
            parse_error(p, type_start, p->cursor - 1, REPORT_ERROR, "type %s differs from previous EXTERN type %s",
                ty_name(decl_ty), ty_name(var->ty));
        }
        if (ty_size(decl_ty) == 0) {
            parse_error(p, type_start, p->cursor - 1, REPORT_ERROR, "variable must have non-zero size");
        }
        var->ty = decl_ty;
        if (match(p, TOK_EQ)) {
            if (storage == STORAGE_EXTERN) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "EXTERN variable cannot have a value");
            }
            advance(p);
            Expr* value = parse_expr(p);
            decl->var_decl.expr = value;
            if (!ty_compatible(decl_ty, value->ty, value->kind == EXPR_LITERAL)) {
                error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
                    ty_name(value->ty), ty_name(decl_ty));
            }
        }
    } else {
        if (storage == STORAGE_EXTERN) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "EXTERN variable must specify a type");
        }
        advance(p);
        Expr* value = parse_expr(p);
        if (var->storage == STORAGE_EXTERN && !ty_compatible(var->ty, value->ty, true)) {
            // parse_error(p, type_start, p->cursor - 1, REPORT_NOTE, "from previous declaration");
            error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to previous EXTERN type %s",
                    ty_name(value->ty), ty_name(var->ty));
        }
        decl->var_decl.expr = value;
        if (var->storage != STORAGE_EXTERN) {
            var->ty = value->ty;
        }
    }

    var->storage = storage;
    var->decl = decl;

    return decl;
}

static bool is_lvalue(Expr* e) {
    switch (e->kind) {
    case EXPR_ENTITY:
        return e->kind == ENTKIND_VAR;
    case EXPR_DEREF:
    case EXPR_DEREF_MEMBER:
    case EXPR_MEMBER:
    case EXPR_SUBSCRIPT:
        return true;
    default:
        return false;
    }
}

Stmt* parse_stmt_assign(Parser* p, Expr* left_expr) {
    Stmt* assign = new_stmt(p, STMT_ASSIGN, assign);
    assign->assign.lhs = left_expr;
    if (!is_lvalue(left_expr)) {
        error_at_expr(p, left_expr, REPORT_ERROR, "expression is not an l-value");
    }

    advance(p);
    Expr* value = parse_expr(p);
    if (!ty_compatible(left_expr->ty, value->ty, value->kind == EXPR_LITERAL)) {
    error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
        ty_name(value->ty), ty_name(left_expr->ty));
    }
    assign->assign.rhs = value;
    return assign;
}

Stmt* parse_stmt_expr(Parser* p) {
        // expression! who knows what this could be
    Expr* expr = parse_expr(p);
    if (TOK_EQ <= p->current.kind && p->current.kind <= TOK_RSHIFT_EQ) {
        // assignment statement
        return parse_stmt_assign(p, expr);
    } else {
        // expression statement
        Stmt* stmt_expr = new_stmt(p, STMT_EXPR, expr);
        stmt_expr->expr = expr;
        stmt_expr->token_index = expr_leftmost_token(expr);
        return stmt_expr;
    }
}

Stmt* parse_stmt(Parser* p) {
    switch (p->current.kind) {
    case TOK_IDENTIFIER:
        if (peek(p, 1).kind == TOK_COLON) {
            return parse_var_decl(p, STORAGE_LOCAL);
        } else {
            return parse_stmt_expr(p);
        }
        break;
    case TOK_KW_NOTHING:
        advance(p);
        return nullptr; // nothing
    case TOK_KW_PUBLIC:
    case TOK_KW_PRIVATE:
    case TOK_KW_EXTERN:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "storage class cannot be used locally");
        break;
    case TOK_KW_TYPE:
    case TOK_KW_STRUCT:
    case TOK_KW_UNION:
    case TOK_KW_ENUM:
    case TOK_KW_FNPTR:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "cannot locally declare type");
        break;
    case TOK_KW_FN:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "cannot locally declare function");
        break;
    default:
        return parse_stmt_expr(p);
    }
    return nullptr;
}

Entity* get_incomplete_type_entity(Parser* p, string identifier) {
    Entity* entity = get_entity(p, identifier);
    if (!entity) {
        entity = new_entity(p, identifier, ENTKIND_TYPE);
        entity->ty = ty_allocate(TyAlias);
        TY_KIND(entity->ty) = TY_ALIAS_INCOMPLETE;
    } else if (entity->kind != ENTKIND_TYPE || TY_KIND(entity->ty) == TY_ALIAS_INCOMPLETE) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "symbol already declared");
    }
    return entity;
}

TyIndex parse_fn_prototype(Parser* p) {
    ArenaState a_save = arena_save(&p->arena);

    usize params_len = 0;
    // temporarily allocate a bunch of function params
    Ty_FnParam* params = arena_alloc(&p->arena, sizeof(Ty_FnParam) * TY_FN_MAX_PARAMS, alignof(Ty_FnParam));

    bool is_variadic = false;

    expect_advance(p,TOK_OPEN_PAREN);
    while (p->current.kind != TOK_CLOSE_PAREN) {
        Ty_FnParam* param = &params[params_len];

        if (match(p, TOK_VARARG)) {
            is_variadic = true;
            advance(p);
            expect(p, TOK_IDENTIFIER);
            string argv = tok_span(p->current);
            param->varargs.argv = to_compact(argv);
            advance(p);
            expect(p, TOK_IDENTIFIER);
            string argc = tok_span(p->current);
            param->varargs.argc = to_compact(argc);
            advance(p);
            params_len++;
            break;
        }


        if (match(p, TOK_KW_IN)) {
            param->out = false;
        } else if (match(p, TOK_KW_OUT)) {
            param->out = true;
        } else {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "expected IN or OUT");
        }
        advance(p);

        expect(p, TOK_IDENTIFIER);
        string ident = tok_span(p->current);
        for_n(i, 0, params_len) {
            if (string_eq(from_compact(params[i].name), ident)) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "parameter name already used");
            }
        }

        param->name = to_compact(ident);

        advance(p);
        expect_advance(p, TOK_COLON);

        param->type = parse_type(p, false);

        params_len++;

        if (match(p, TOK_COMMA)) {
            advance(p);
        } else {
            break;
        }
    }
    expect_advance(p, TOK_CLOSE_PAREN);

    TyIndex ret_ty = TY_VOID;

    if (match(p, TOK_COLON)) {
        advance(p);
        ret_ty = parse_type(p, false);
    }

    // allocate final type
    TyIndex proto = ty__allocate(sizeof(TyFn) + sizeof(Ty_FnParam) * params_len, max(alignof(TyFn), alignof(Ty_FnParam)) == 8);
    TyFn* fn = TY(proto, TyFn);
    fn->kind = TY_FN;
    fn->len = params_len;
    fn->variadic = is_variadic;
    fn->ret_ty = ret_ty;
    memcpy(fn->params, params, sizeof(Ty_FnParam) * params_len);

    arena_restore(&p->arena, a_save);
    return proto;
}

Stmt* parse_fn_decl(Parser* p, u8 storage) {
    // advance(p);
    advance(p);
    // no fnptr specifier yet
    expect(p, TOK_IDENTIFIER);
    u32 ident_pos = p->cursor;
    string identifier = tok_span(p->current);
    Entity* fn = get_or_create(p, identifier);
    if (fn->storage == STORAGE_EXTERN && storage == STORAGE_PRIVATE) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "previously EXTERN function cannot be PRIVATE");
    }
    advance(p);
    TyIndex decl_ty = parse_fn_prototype(p);
    if (fn->storage == STORAGE_EXTERN && !ty_equal(fn->ty, decl_ty)) {
        parse_error(p, ident_pos, ident_pos, REPORT_ERROR, "type %s differs from previous EXTERN type %s",
            ty_name(decl_ty), ty_name(fn->ty));
    }
    if (storage != STORAGE_EXTERN) {
        Stmt* fn_decl = new_stmt(p, STMT_FN_DECL, fn_decl);
        fn_decl->fn_decl.fn = fn;

        u32 stmts_start = dynbuf_start();
        u32 stmts_len = 0;
        while (!match(p, TOK_KW_END)) {
            Stmt* stmt = parse_stmt(p);
            if (stmt == nullptr) {
                continue;
            }
            vec_append(&dynbuf, stmt);
            stmts_len++;
        }
        Stmt** stmts = (Stmt**)dynbuf_to_arena(p, stmts_start);
        
        dynbuf_restore(stmts_start);
        
    }
    fn->storage = storage;
    return nullptr;
}

Stmt* parse_global_decl(Parser* p) {
    switch (p->current.kind) {
    case TOK_KW_NOTHING:
        advance(p);
        return parse_global_decl(p);
    case TOK_KW_TYPE: {
        advance(p);
        expect(p, TOK_IDENTIFIER);
        string identifier = tok_span(p->current);
        Entity* entity = get_incomplete_type_entity(p, identifier);
        advance(p);
        expect_advance(p, TOK_COLON);
        TY(entity->ty, TyAlias)->aliasing = parse_type(p, false);
        TY_KIND(entity->ty) = TY_ALIAS;
    } break;
    case TOK_KW_PUBLIC:
    case TOK_KW_PRIVATE:
    case TOK_KW_EXPORT:
    case TOK_KW_EXTERN:
        ;
        u64 effective_storage = p->current.kind - TOK_KW_PUBLIC + STORAGE_PUBLIC;
        advance(p);
        if (p->current.kind == TOK_KW_FN) {
            return parse_fn_decl(p, effective_storage);
        }
        return parse_var_decl(p, effective_storage);
    case TOK_IDENTIFIER:
        return parse_var_decl(p, STORAGE_PRIVATE);
    case TOK_KW_FN:
        return parse_fn_decl(p, STORAGE_PUBLIC);
    default:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "expected global declaration");
    }
    return nullptr;
}

CompilationUnit parse_unit(Parser* p) {
    ty_init();

    dynbuf = vecptr_new(void, 256);

    while (p->current.kind != TOK_EOF) {
        parse_global_decl(p);
    }

    CompilationUnit cu = {};
    cu.tokens = p->tokens;
    cu.tokens_len = p->tokens_len;
    cu.sources = p->sources;
    cu.top_scope = p->global_scope;
    cu.arena = p->arena;

    vec_destroy(&dynbuf);

    return cu;
}
