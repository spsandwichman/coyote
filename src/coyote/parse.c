#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "common/str.h"
#include "common/util.h"
#include "common/vec.h"
#include "coyote.h"
#include "lex.h"

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

static bool ty_is_scalar(TyIndex t) {
    if (t != TY_VOID && t < TY_PTR) {
        return true;
    }
    u8 ty_kind = TY(t, TyBase)->kind;
    if (ty_kind == TY_PTR || ty_kind == TY_FNPTR) {
        return true;
    }
    return false;
}

thread_local static usize target_ptr_size = 4;
thread_local static TyIndex target_word  = TY_LONG;
thread_local static TyIndex target_uword = TY_ULONG;

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
    default:
        break;
    }
    if (TY_KIND(t) == TY_PTR) {
        return target_ptr_size;
    }
    TODO("AAAA");
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

Vec_typedef(char);

static void vec_char_append_many(Vec(char)* vec, const char* data, usize len) {
    for_n(i, 0, len) {
        vec_append(vec, data[i]);
    }
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

void p_advance(Parser* p) {
    do { // skip past "transparent" tokens.
        ++p->cursor;
    } while (_TOK_LEX_IGNORE < p->tokens[p->cursor].kind);
    p->current = p->tokens[p->cursor];
}

bool match(Parser* p, u8 kind) {
    return p->current.kind == kind;
}

void parse_error(Parser* p, u32 index, ReportKind kind, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    thread_local static char buf[128];
    vsprintf_s(buf, sizeof(buf), fmt, args);

    va_end(args);

    token_error(p, kind, index, index, buf);
}

void expect_advance(Parser* p, u8 kind) {
    if (kind != p->current.kind) {
        parse_error(p, REPORT_ERROR, p->cursor, "expected '%s', got '%s'", token_kind[kind], token_kind[p->current.kind]);
    }
    p_advance(p);
}

static Expr* new_expr_(Parser* p, u8 kind, TyIndex ty, usize size) {
    Expr* expr = arena_alloc(&p->arena, size, alignof(Expr));
    expr->kind = kind;
    expr->ty = ty;
    expr->token_index = p->cursor;
    return expr;
}

#define new_expr(p, kind, ty, field) \
    new_expr_(p, kind, ty, offsetof(Expr, extra) + sizeof(((Expr*)nullptr)->field))

static i64 eval_integer(Parser* p, Token t, u32 index) {
    i64 val = 0;
    char* raw = tok_raw(t);
    bool is_negative = raw[0] == '-';

    for (usize i = is_negative; i < t.len; ++i) {
        val *= 10;
        isize char_val = raw[i] - '0';
        if (char_val < 0 || char_val > 9) {
            // TODO("invalid digit");
            parse_error(p, index, REPORT_ERROR, "invalid digit '%c'", raw[i]);
        }
        val += raw[i] - '0';
    }

    return is_negative ? -val : val;
}

TyIndex parse_type_terminal(Parser* p) {
    switch (p->current.kind) {
    case TOK_KW_VOID:  return TY_VOID;
    case TOK_KW_BYTE:  return TY_BYTE;
    case TOK_KW_UBYTE: return TY_UBYTE;
    case TOK_KW_INT:   return TY_INT;
    case TOK_KW_UINT:  return TY_UINT;
    case TOK_KW_LONG:  return TY_LONG;
    case TOK_KW_ULONG: return TY_ULONG;
    case TOK_KW_QUAD:  return TY_QUAD;
    case TOK_KW_UQUAD: return TY_UQUAD;
    case TOK_KW_WORD:  return target_word;
    case TOK_KW_UWORD: return target_uword;
    case TOK_OPEN_PAREN:
        p_advance(p);
        TyIndex inner = parse_type(p);
        expect_advance(p, TOK_CLOSE_PAREN);
        return inner;
    case TOK_CARET:
        p_advance(p);
        return ty_get_ptr(parse_type_terminal(p));
    default:
        parse_error(p, p->cursor, REPORT_ERROR, "expected type");
    }
    return TY_VOID;
}

TyIndex parse_type(Parser* p) {
    return parse_type_terminal(p);
}

Expr* parse_atom_terminal(Parser* p) {
    string span = tok_span(p->current);
    switch (p->current.kind) {
    case TOK_OPEN_PAREN:
        p_advance(p);
        Expr* inner = parse_expr(p);
        expect_advance(p, TOK_CLOSE_PAREN);
        return inner;
    case TOK_INTEGER:
        Expr* int_lit = new_expr(p, EXPR_LITERAL, target_uword, literal);
        int_lit->literal = eval_integer(p, p->current, p->cursor);
        p_advance(p);
        return int_lit;
    case TOK_KW_TRUE:
        Expr* true_lit = new_expr(p, EXPR_LITERAL, target_uword, literal);
        true_lit->literal = 1;
        p_advance(p);
        return true_lit;
    case TOK_KW_FALSE:
        Expr* false_lit = new_expr(p, EXPR_LITERAL, target_uword, literal);
        false_lit->literal = 0;
        p_advance(p);
        return false_lit;
    case TOK_KW_NULLPTR:
        Expr* nullptr_lit = new_expr(p, EXPR_LITERAL, ty_get_ptr(TY_VOID), literal);
        nullptr_lit->literal = 0;
        p_advance(p);
        return nullptr_lit;
    case TOK_KW_SIZEOF:
        Expr* size = new_expr(p, EXPR_LITERAL, target_uword, literal);
        p_advance(p);
        TyIndex type = parse_type(p); 
        size->literal = ty_size(type);
        return size;
    default:
        parse_error(p, p->cursor, REPORT_ERROR, "expected expression, got '%s'", token_kind[p->current.kind]);
    }

    return nullptr;
}

Expr* parse_expr(Parser* p) {
    return parse_atom_terminal(p);
}
