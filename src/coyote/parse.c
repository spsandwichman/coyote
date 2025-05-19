#include <stdint.h>
#include <string.h>

#include "parse.h"
#include "common/str.h"
#include "common/util.h"
#include "common/vec.h"
#include "iron/iron.h"
#include "lex.h"

static struct {
    TyBufSlot* at;
    TyIndex* ptrs;
    u32 len;
    u32 cap;
} tybuf;

#define TY(index, T) ((T*)&tybuf.at[index])
#define TY_KIND(index) ((TyBase*)&tybuf.at[index])->kind

static void tg_init() {
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

#define tg_allocate(T) tg__allocate(sizeof(T), alignof(T) == 8)
static TyIndex tg__allocate(usize size, bool align64) {
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

static TyIndex tg_get_ptr(TyIndex t) {
    if (t < TY_PTR) {
        return t + TY_PTR;
    }
    if (tybuf.ptrs[t] == 0) {
        // we have to create a pointer type since none exists.
        TyIndex ptr = tg_allocate(TyPtr);
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

static usize target_ptr_size = 4;
static TyIndex target_intptr  = TY_LONG;
static TyIndex target_uintptr = TY_ULONG;
static FeTy target_fe_uintptr = FE_TY_I32;

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

static SrcFile* where_from(Context* ctx, string span) {
    for_n (i, 0, ctx->sources.len) {
        SrcFile* f = ctx->sources.at[i];
        if (token_is_within(f, span.raw)) {
            return f;
        }
    }
    return nullptr;
}

Vec_typedef(char);

void vec_char_append_many(Vec(char)* vec, const char* data, usize len) {
    for_n(i, 0, len) {
        vec_append(vec, data[i]);
    }
}

static usize preproc_depth(Context* ctx, u32 index) {
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

void token_error(Context* ctx, u32 start_index, u32 end_index, const char* msg) {
    // find out if we're in a macro somewhere
    bool inside_preproc = preproc_depth(ctx, start_index) != 0 || preproc_depth(ctx, end_index) != 0;

    Vec_typedef(ReportLine);
    Vec(ReportLine) reports = vec_new(ReportLine, 8);

    i32 unmatched_ends = 0;
    for (i64 i = (i64)start_index; i > 0; --i) {
        Token t = ctx->tokens[i];

        ReportLine report;
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
        string main_highlight = {};
        for_n(i, end_index, ctx->tokens_len) {
            Token t = ctx->tokens[i];
            if (t.kind == TOK_PREPROC_PASTE_END && token_is_within(main_file, tok_raw(t))) {
                main_highlight = tok_span(t);
                break;
            }
        }
        ReportLine main_line_report = {
            .kind = REPORT_NOTE,
            .msg = str(msg),
            .path = main_file->path,
            .src = main_file->src,
            .snippet = main_highlight,
        };

        vec_append(&reports, main_line_report);

        for_n(i, 0, reports.len) {
            report_line(&reports.at[i]);
        }

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
            .kind = REPORT_ERROR,
            .msg = str(msg),
            .path = main_file->path,
            .snippet = snippet,
            .src = src,
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
            .kind = REPORT_ERROR,
            .msg = str(msg),
            .path = main_file->path,
            .snippet = span,
            .src = main_file->src,
        };

        report_line(&rep);
    }


    // printf(str_fmt"\n", (int)expanded_snippet.len, expanded_snippet.at);
    // for_n(i, 0, (usize)expanded_snippet_highlight_start) {
    //     printf(" ");
    // }
    // printf("^");
    // for_n(i, 1, expanded_snippet_highlight_len) {
    //     printf("~");
    // }
    // printf("\n");
    // exit(0);
}

void p_advance(Parser* p) {
    do { // skip past "transparent" tokens.
        ++p->cursor;
    } while (_TOK_LEX_IGNORE < p->tokens[p->cursor].kind);
    p->current = p->tokens[p->cursor];
}
