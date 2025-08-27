#include "lex.h"
#include "common/strmap.h"

#include <ctype.h>

static char current(Lexer* l) {
    return l->src.raw[l->cursor];
}

static char peek(Lexer* l, int n) {
    if (l->cursor + n >= l->src.len) {
        return '\0';
    } else {
        return l->src.raw[l->cursor];
    }
}

static char* current_ptr(Lexer* l) {
    return &l->src.raw[l->cursor];
}

static bool is_eof(Lexer* l) {
    return l->cursor >= l->src.len;
}

static void skip_whitespace(Lexer* l) {
    while (l->cursor < l->src.len) {
        switch (current(l)) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case '\v':
            l->cursor += 1;
            break;
        default:
            return;
        }
    }
}

static Token construct_advance(Lexer* l, TokenKind kind, u16 len) {
    Token t = {
        .data = current_ptr(l),
        .len = len,
        .kind = kind,
    };
    l->cursor += len;
    return t;
}

static StrMap kw_map;

static void init_kw_map() {
    strmap_init(&kw_map, 256);
    
    strmap_put(&kw_map, strlit("module"), (void*)TOK_KW_MODULE);
    strmap_put(&kw_map, strlit("common"), (void*)TOK_KW_COMMON);
    strmap_put(&kw_map, strlit("threadlocal"), (void*)TOK_KW_THREADLOCAL);
    strmap_put(&kw_map, strlit("extern"), (void*)TOK_KW_EXTERN);
    strmap_put(&kw_map, strlit("builtin"), (void*)TOK_KW_BUILTIN);
    strmap_put(&kw_map, strlit("pub"), (void*)TOK_KW_PUB);
    strmap_put(&kw_map, strlit("def"), (void*)TOK_KW_DEF);
    strmap_put(&kw_map, strlit("let"), (void*)TOK_KW_LET);
    strmap_put(&kw_map, strlit("mut"), (void*)TOK_KW_MUT);
    strmap_put(&kw_map, strlit("true"), (void*)TOK_KW_TRUE);
    strmap_put(&kw_map, strlit("false"), (void*)TOK_KW_FALSE);
    strmap_put(&kw_map, strlit("null"), (void*)TOK_KW_NULL);
    strmap_put(&kw_map, strlit("if"), (void*)TOK_KW_IF);
    strmap_put(&kw_map, strlit("else"), (void*)TOK_KW_ELSE);
    strmap_put(&kw_map, strlit("while"), (void*)TOK_KW_WHILE);
    strmap_put(&kw_map, strlit("for"), (void*)TOK_KW_FOR);
    strmap_put(&kw_map, strlit("in"), (void*)TOK_KW_IN);
    strmap_put(&kw_map, strlit("as"), (void*)TOK_KW_AS);
    strmap_put(&kw_map, strlit("inline"), (void*)TOK_KW_INLINE);
    strmap_put(&kw_map, strlit("void"), (void*)TOK_KW_VOID);
    strmap_put(&kw_map, strlit("bool"), (void*)TOK_KW_BOOL);
    strmap_put(&kw_map, strlit("f32"), (void*)TOK_KW_F32);
    strmap_put(&kw_map, strlit("f64"), (void*)TOK_KW_F64);
    strmap_put(&kw_map, strlit("type"), (void*)TOK_KW_TYPE);
    strmap_put(&kw_map, strlit("struct"), (void*)TOK_KW_STRUCT);
    strmap_put(&kw_map, strlit("union"), (void*)TOK_KW_UNION);
    strmap_put(&kw_map, strlit("enum"), (void*)TOK_KW_ENUM);
    strmap_put(&kw_map, strlit("noreturn"), (void*)TOK_KW_NORETURN);
    strmap_put(&kw_map, strlit("usize"), (void*)TOK_KW_USIZE);
    strmap_put(&kw_map, strlit("isize"), (void*)TOK_KW_ISIZE);
}

static usize scan_integer(Lexer* l, usize start) {
    
}

Token lex_next(Lexer* l) {
    skip_whitespace(l);
    if (is_eof(l)) {
        return (Token){
            .kind = TOK_EOF,
            .data = &l->src.raw[l->src.len - 1],
            .len = 1,
        };
    }
    
    switch (current(l)) {
    case '(': return construct_advance(l, TOK_OPEN_PAREN, 1);
    case ')': return construct_advance(l, TOK_CLOSE_PAREN, 1);
    case '{': return construct_advance(l, TOK_OPEN_BRACE, 1);
    case '}': return construct_advance(l, TOK_CLOSE_BRACE, 1);
    case '[': return construct_advance(l, TOK_OPEN_BRACKET, 1);
    case ']': return construct_advance(l, TOK_CLOSE_BRACKET, 1);

    case '#': return construct_advance(l, TOK_HASH, 1);
    case ';': return construct_advance(l, TOK_SEMICOLON, 1);
    case '?': return construct_advance(l, TOK_QUESTION, 1);
    case ':': return construct_advance(l, TOK_COLON, 1);
    case ',': return construct_advance(l, TOK_COMMA, 1);
    case '!':
        if (peek(l, 1) == '=') {
            return construct_advance(l, TOK_NOT_EQ, 2);
        } else {
            return construct_advance(l, TOK_EXCLAM, 1);
        }

    case 'u':
        if (isdigit(peek(l, 1))) {
            while (isdigit(peek(l, start))) {
                
            }
        }
        break;
    }
    // english
}