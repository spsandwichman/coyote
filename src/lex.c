#include "front.h"

static u8 lex_categorize_keyword(char* s, size_t len);

void lexer_error(Lexer* l, isize n, char* fmt, ...) {
    va_list varargs;
    va_start(varargs, fmt);
    emit_report(true, l->text, l->path, (string){l->text.raw + l->cursor + n, 1}, fmt, varargs);
}

void lex_advance(Lexer* l) {
    l->cursor++;
    if (l->cursor > l->text.len) {
        l->cursor = l->text.len;
        l->current = '\0';
    } else {
        l->current = l->text.raw[l->cursor];
    }
}

void lex_advance_n(Lexer* l, int n) {
    l->cursor += n;
    if (l->cursor > l->text.len) {
        l->cursor = l->text.len;
        l->current = '\0';
    } else {
        l->current = l->text.raw[l->cursor];    
    }
}

char lex_peek(Lexer* l, i64 n) {
    if (l->cursor + n >= l->text.len) return '\0';
    return l->text.raw[l->cursor + n];
}

bool lex_cursor_eof(Lexer* l) {
    return l->cursor >= l->text.len;
}

void lex_skip_whitespace(Lexer* l) {
    while (!lex_cursor_eof(l)) {
        switch (l->current) {
        case ' ':
        case '\t':
        case '\r':
        case '\v':
        case '\n':
        case '\f':
            lex_advance(l);
            break;
        default: 
            return;
        }
    }
}

void lex_add_token(Lexer* l, u16 len, u8 kind) {
    Token t;
    t.raw = &l->text.raw[l->cursor];
    t.len = len;
    t.kind = kind;
    da_append(l->tb, t);
}

bool lex_can_begin_ident(char c) {
    return ('A' <= c && c <= 'Z') ||
           ('a' <= c && c <= 'z') ||
           c == '_';
}

bool lex_can_ident(char c) {
    return ('A' <= c && c <= 'Z') ||
           ('a' <= c && c <= 'z') ||
           ('0' <= c && c <= '9') ||
           c == '_';
}

u64 lex_scan_ident(Lexer* l) {
    u64 len = 1;
    while (lex_can_ident(lex_peek(l, len))) {
        len++;
    }
    return len;
}

bool lex_is_numeric(char c) {
    return ('0' <= c && c <= '9');
}

bool lex_is_base_digit(char c, u64 base) {
    if (c == '_') return true;

    if (base <= 10) {
        return '0' <= c && c <= ('0' + base - 1);
    } else {
        return 
            ('0' <= c && c <= '9') || 
            ('a' <= c && c <= ('a' + base - 10)) ||
            ('A' <= c && c <= ('A' + base - 10));
    }
}

u64 lex_scan_integer_base(Lexer* l, u64 base, u64 start_at) {
    u64 len = start_at;
    while (lex_is_base_digit(lex_peek(l, len), base)) {
        len++;
    }
    if (lex_can_ident(lex_peek(l, len))) {
        CRASH("invalid digit");
    }
    return len;
}

u64 lex_scan_numeric(Lexer* l) {
    if (l->current == '0') {
        switch (lex_peek(l, 1)){
        case 'x':
        case 'X': return lex_scan_integer_base(l, 16, 2);
        case 'd':
        case 'D': return lex_scan_integer_base(l, 10, 2);
        case 'o':
        case 'O': return lex_scan_integer_base(l, 8, 2);
        case 'b':
        case 'B': return lex_scan_integer_base(l, 2, 2);
        default:
            if (lex_is_numeric(lex_peek(l, 1))) {
                return lex_scan_integer_base(l, 10, 2);
            } else if (lex_can_ident(lex_peek(l, 1))) {
                lexer_error(l, 1, "invalid digit or base signifier");
            }
            return 1;
        }
    }
    if (lex_is_numeric(l->current)) {
        return lex_scan_integer_base(l, 10, 1);
    }
    return 0;
}

#define push_simple_token(kind) do { lex_add_token(l, 1, kind); lex_advance(l); goto next_token;} while (0)
#define push_token(kind, len) do { lex_add_token(l, len, kind); lex_advance_n(l, len); goto next_token;} while (0)

static void tokenize(Lexer* l) {
    if (lex_cursor_eof(l)) {
        lex_add_token(l, 0, TOK_EOF);
        return;
    }
    
    while (!lex_cursor_eof(l)) {
        lex_skip_whitespace(l);
        if (lex_cursor_eof(l)) {
            lex_add_token(l, 0, TOK_EOF);
            return;
        }

        // printf("%d '%c'\n", l->cursor, l->current);

        switch (l->current) {        
        case '#': 
            preproc_dispatch(l);
            break;
        case '(':  push_simple_token(TOK_OPEN_PAREN);
        case ')':  push_simple_token(TOK_CLOSE_PAREN);
        case '[':  push_simple_token(TOK_OPEN_BRACKET);
        case ']':  push_simple_token(TOK_CLOSE_BRACKET);
        case '{':  push_simple_token(TOK_OPEN_BRACE);
        case '}':  push_simple_token(TOK_CLOSE_BRACE);
        case ',':  push_simple_token(TOK_COMMA);
        case '^':  push_simple_token(TOK_CARET);
        case '@':  push_simple_token(TOK_AT);
        case '.':
            if (lex_peek(l, 1) == '.' && lex_peek(l, 2) == '.') {
                push_token(TOK_VARARG, 3);
            }
            push_simple_token(TOK_DOT);
        case '&':
            if (lex_peek(l, 1) == '&') 
                push_token(TOK_AND, 2);
            if (lex_peek(l, 1) == '=') 
                push_token(TOK_AND_EQ, 2);
            push_simple_token(TOK_AND);
        case '|':
            if (lex_peek(l, 1) == '=') 
                push_token(TOK_OR_EQ, 2);
            push_simple_token(TOK_OR);
        case ':':
            push_simple_token(TOK_COLON);
        case '!':
            if (lex_peek(l, 1) == '=')
                push_token(TOK_NOT_EQ, 2);
            break;
        case '+':
            if (lex_peek(l, 1) == '=')
                push_token(TOK_PLUS_EQ, 2);
            push_simple_token(TOK_PLUS);
        case '*':
            if (lex_peek(l, 1) == '=')
                push_token(TOK_MUL_EQ, 2);
            push_simple_token(TOK_MUL);
        case '~':
            push_simple_token(TOK_TILDE);
        case '/':
            if (lex_peek(l, 1) == '/') {
                lex_advance_n(l, 2);
                while (l->current != '\n' && l->current != '\0') {
                    lex_advance(l);
                }
                goto next_token;
            }
            if (lex_peek(l, 1) == '*') {
                lex_advance_n(l, 2);
                int level = 1;
                while (level != 0 && l->current != '\0') {
                    if (l->current == '/' && lex_peek(l, 1) == '*') {
                        level += 1;
                        lex_advance_n(l, 2);
                        continue;
                    } else if (l->current == '*' && lex_peek(l, 1) == '/') {
                        level -= 1;
                        lex_advance_n(l, 2);
                        continue;
                    }
                    lex_advance(l);
                }
                goto next_token;
            }
            if (lex_peek(l, 1) == '=')
                push_token(TOK_DIV_EQ, 2);
            push_simple_token(TOK_DIV);
        case '%':
            if (lex_peek(l, 1) == '=')
                push_token(TOK_MOD_EQ, 2);
            push_simple_token(TOK_MOD);
        case '-':
            if (lex_peek(l, 1) == '=')
                push_token(TOK_MINUS_EQ, 2);
            push_simple_token(TOK_MINUS);
        case '<':
            if (lex_peek(l, 1) == '<' && lex_peek(l, 2) == '=')
                push_token(TOK_LSHIFT_EQ, 3);
            if (lex_peek(l, 1) == '<')
                push_token(TOK_LSHIFT, 2);
            if (lex_peek(l, 1) == '=')
                push_token(TOK_LESS_EQ, 2);
            push_simple_token(TOK_LESS);
        case '>':
            if (lex_peek(l, 1) == '>' && lex_peek(l, 2) == '=')
                push_token(TOK_RSHIFT_EQ, 3);
            if (lex_peek(l, 1) == '>')
                push_token(TOK_RSHIFT, 2);
            if (lex_peek(l, 1) == '=')
                push_token(TOK_GREATER_EQ, 2);
            push_simple_token(TOK_GREATER);
        case '=':
            if (lex_peek(l, 1) == '=')
                push_token(TOK_EQ_EQ, 2);
            push_simple_token(TOK_EQ);
        case '\'':
            u64 len = 1;
            while (!(lex_peek(l, len) == '\'' && lex_peek(l, len - 1) != '\\')) {
                if (lex_peek(l, len) == '\0') {
                    CRASH("unterminated char literal");
                }
                len++;
            }
            // validity will be checked at sema-time
            lex_add_token(l, len, TOK_CHAR);
            lex_advance_n(l, len + 1);
            goto next_token;
        case '\"':
            len = 1;
            while (!(lex_peek(l, len) == '\"' && lex_peek(l, len - 1) != '\\')) {
                if (lex_peek(l, len) == '\0') {
                    CRASH("unterminated string literal");
                }
                len++;
            }
            // validity will be checked at sema-time
            lex_add_token(l, len, TOK_STRING);
            lex_advance_n(l, len + 1);
            goto next_token;
        }

        if (lex_can_begin_ident(l->current)) {
            u64 len = lex_scan_ident(l);
            u8 kind = lex_categorize_keyword(&l->text.raw[l->cursor], len);
            lex_add_token(l, len, kind);
            lex_advance_n(l, len);
            goto next_token;
        }

        if (lex_is_numeric(l->current)) {
            u64 len = lex_scan_numeric(l);
            lex_add_token(l, len, TOK_NUMERIC);
            lex_advance_n(l, len);
            goto next_token;
        }

        printf("unrecognized '%c' %d\n", l->current, l->cursor);
        CRASH("unrecognized");
        return;
        
        next_token:
    }
}


TokenBuf lex_tokenize(SourceFile* src) {
    TokenBuf tb = {0};
    da_init(&tb, 128);

    Lexer l = {
        .cursor = 0,
        .tb = &tb,
        .text = src->text,
        .path = src->path,
        .current = src->text.len == 0 ? '\0' : src->text.raw[0],
    };

    tokenize(&l);

    lex_add_token(&l, 0, TOK_EOF);
    
    return tb;
}

#define return_if_eq(kind) if (!strncmp(s, #kind, sizeof(#kind)-1)) return TOK_KEYWORD_##kind

u8 lex_categorize_keyword(char* s, size_t len) {
    switch (len) {
    case 2:
        return_if_eq(DO);
        return_if_eq(FN);
        return_if_eq(IF);
        return_if_eq(IN);
        return_if_eq(OR);
        return_if_eq(TO);
        break;
    case 3:
        return_if_eq(AND);
        return_if_eq(END);
        return_if_eq(INT);
        return_if_eq(NOT);
        return_if_eq(OUT);
        break;
    case 4:
        return_if_eq(BYTE);
        return_if_eq(CAST);
        return_if_eq(ELSE);
        return_if_eq(ENUM);
        return_if_eq(GOTO);
        return_if_eq(LONG);
        return_if_eq(THEN);
        return_if_eq(TRUE);
        return_if_eq(TYPE);
        return_if_eq(UINT);
        return_if_eq(VOID);
        return_if_eq(QUAD);
        break;
    case 5:
        return_if_eq(BREAK);
        return_if_eq(FALSE);
        return_if_eq(FNPTR);
        return_if_eq(LEAVE);
        return_if_eq(UBYTE);
        return_if_eq(ULONG);
        return_if_eq(UNION);
        return_if_eq(WHILE);
        return_if_eq(PROBE);
        return_if_eq(UQUAD);
        break;
    case 6:
        return_if_eq(ELSEIF);
        return_if_eq(EXTERN);
        return_if_eq(PACKED);
        return_if_eq(PUBLIC);
        return_if_eq(RETURN);
        return_if_eq(SIZEOF);
        return_if_eq(STRUCT);
        return_if_eq(EXPORT);
        break;
    case 7:
        return_if_eq(NULLPTR);
        return_if_eq(BARRIER);
        return_if_eq(NOTHING);
        return_if_eq(PRIVATE);
        break;
    case 8:
        return_if_eq(CONTINUE);
        return_if_eq(OFFSETOF);
        break;
    case 11:
        return_if_eq(CONTAINEROF);
        return_if_eq(SIZEOFVALUE);
        break;
    }
    return TOK_IDENTIFIER;
}

const char* token_kind[_TOK_COUNT] = {

    [_TOK_INVALID] = "[invalid]",
    [TOK_EOF] = "EOF",

    [TOK_OPEN_PAREN] =    "(",
    [TOK_CLOSE_PAREN] =   ")",
    [TOK_OPEN_BRACE] =    "{",
    [TOK_CLOSE_BRACE] =   "}",
    [TOK_OPEN_BRACKET] =  "[",
    [TOK_CLOSE_BRACKET] = "]",

    [TOK_COLON] =      ":",
    [TOK_COMMA] =      ",",
    [TOK_EQ_EQ] =      "==",
    [TOK_NOT_EQ] =     "!=",
    [TOK_AND] =        "&=",
    [TOK_OR] =         "|",
    [TOK_LESS] =       "<",
    [TOK_GREATER] =    ">",
    [TOK_LESS_EQ] =    "<=",
    [TOK_GREATER_EQ] = ">=",
    [TOK_PLUS] =       "+",
    [TOK_MINUS] =      "-",
    [TOK_MUL] =        "*",
    [TOK_DIV] =        "/",
    [TOK_MOD] =        "%",
    [TOK_DOT] =        ".",
    [TOK_AT] =         "@",
    [TOK_DOLLAR] =     "$",
    [TOK_LSHIFT] =     "<<",
    [TOK_RSHIFT] =     ">>",
    [TOK_TILDE] =      "~",
    [TOK_VARARG] =     "...",
    [TOK_CARET] =      "^",

    [TOK_EQ] =        "=",
    [TOK_PLUS_EQ] =   "+=",
    [TOK_MINUS_EQ] =  "-=",
    [TOK_MUL_EQ] =    "*=",
    [TOK_DIV_EQ] =    "/=",
    [TOK_MOD_EQ] =    "%=",
    [TOK_AND_EQ] =    "&=",
    [TOK_OR_EQ] =     "|=",
    [TOK_DOLLAR_EQ] = "$=",
    [TOK_LSHIFT_EQ] = "<<=",
    [TOK_RSHIFT_EQ] = ">>=",

    [TOK_IDENTIFIER] = "identifier",

    [TOK_CHAR] = "char",
    [TOK_STRING] = "string",

    [TOK_NUMERIC] = "numeric",

    #define T(kw) [TOK_KEYWORD_##kw] = #kw
        _LEX_KEYWORDS_
    #undef T
};