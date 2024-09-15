#include "front.h"

static u8 categorize_keyword(char* s, size_t len);

static void advance(Lexer* l) {
    l->cursor++;
    if (l->cursor > l->text.len) {
        l->cursor = l->text.len;
        l->current = '\0';
    } else {
        l->current = l->text.raw[l->cursor];
    }
}

static void advance_n(Lexer* l, int n) {
    l->cursor += n;
    if (l->cursor > l->text.len) {
        l->cursor = l->text.len;
        l->current = '\0';
    } else {
        l->current = l->text.raw[l->cursor];    
    }
}

static bool cursor_eof(Lexer* l) {
    return l->cursor >= l->text.len;
}

static void skip_whitespace(Lexer* l) {
    while (!cursor_eof(l)) {
        switch (l->current) {
        case ' ':
        case '\t':
        case '\r':
        case '\v':
        case '\n':
        case '\f':
            advance(l);
            break;
        default: 
            return;
        }
    }
}

static void add_token(Lexer* l, u16 len, u8 kind) {
    Token t;
    t.raw = &l->text.raw[l->cursor];
    t.len = len;
    t.kind = kind;
    da_append(l->tb, t);
}

static bool can_begin_ident(char c) {
    return ('A' <= c && c <= 'Z') ||
           ('a' <= c && c <= 'z') ||
           c == '_';
}

static bool can_ident(char c) {
    return ('A' <= c && c <= 'Z') ||
           ('a' <= c && c <= 'z') ||
           ('0' <= c && c <= '9') ||
           c == '_';
}

static bool is_numeric(char c) {
    return ('0' <= c && c <= '9');
}

static char peek(Lexer* l, i64 n) {
    if (l->cursor + n >= l->text.len) return '\0';
    return l->text.raw[l->cursor + n];
}

static bool is_base_digit(char c, u64 base) {
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

static u64 scan_integer_base(Lexer* l, u64 base, u64 start_at) {
    u64 len = start_at;
    while (is_base_digit(peek(l, len), base)) {
        len++;
    }
    if (can_ident(peek(l, len))) {
        CRASH("invalid digit");
    }
    return len;
}

static u64 scan_numeric(Lexer* l) {
    if (l->current == '0') {
        switch (peek(l, 1)){
        case 'x':
        case 'X': return scan_integer_base(l, 16, 2);
        case 'd':
        case 'D': return scan_integer_base(l, 10, 2);
        case 'o':
        case 'O': return scan_integer_base(l, 8, 2);
        case 'b':
        case 'B': return scan_integer_base(l, 2, 2);
        default:
            if (is_numeric(peek(l, 1))) {
                return scan_integer_base(l, 10, 2);
            }
            break;
        }
    }
    if (is_numeric(l->current)) {
        return scan_integer_base(l, 10, 1);
    }
    CRASH("what?");
}

#define push_simple_token(kind) do { add_token(l, 1, kind); advance(l); goto next_token;} while (0)
#define push_token(kind, len) do { add_token(l, len, kind); advance_n(l, len); goto next_token;} while (0)

static void tokenize(Lexer* l) {
    if (cursor_eof(l)) {
        add_token(l, 1, TOK_EOF);
        return;
    }
    
    while (!cursor_eof(l)) {
        skip_whitespace(l);
        if (cursor_eof(l)) {
            add_token(l, 1, TOK_EOF);
            return;
        }

        // printf("%d '%c'\n", l->cursor, l->current);

        switch (l->current) {        
        case '#': 
            TODO("preprocessor");
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
            if (is_numeric(peek(l, 1))) {
                TODO("numbers!");
            }
            push_simple_token(TOK_DOT);
        case '&':
            if (peek(l, 1) == '&') 
                push_token(TOK_AND, 2);
            if (peek(l, 1) == '=') 
                push_token(TOK_AND_EQ, 2);
            push_simple_token(TOK_AND);
        case '|':
            if (peek(l, 1) == '=') 
                push_token(TOK_OR_EQ, 2);
            push_simple_token(TOK_OR);
        case ':':
            push_simple_token(TOK_COLON);
        case '!':
            if (peek(l, 1) == '=')
                push_token(TOK_NOT_EQ, 2);
            break;
        case '+':
            if (peek(l, 1) == '=')
                push_token(TOK_PLUS_EQ, 2);
            push_simple_token(TOK_PLUS);
        case '*':
            if (peek(l, 1) == '=')
                push_token(TOK_MUL_EQ, 2);
            push_simple_token(TOK_MUL);
        case '~':
            push_simple_token(TOK_TILDE);
        case '/':
            if (peek(l, 1) == '/') {
                advance_n(l, 2);
                while (l->current != '\n' && l->current != '\0') {
                    advance(l);
                }
                goto next_token;
            }
            if (peek(l, 1) == '*') {
                advance_n(l, 2);
                int level = 1;
                while (level != 0 && l->current != '\0') {
                    if (l->current == '/' && peek(l, 1) == '*') {
                        level += 1;
                        advance_n(l, 2);
                        continue;
                    } else if (l->current == '*' && peek(l, 1) == '/') {
                        level -= 1;
                        advance_n(l, 2);
                        continue;
                    }
                    advance(l);
                }
                goto next_token;
            }
            if (peek(l, 1) == '=')
                push_token(TOK_DIV_EQ, 2);
            push_simple_token(TOK_DIV);
        case '%':
            if (peek(l, 1) == '=')
                push_token(TOK_MOD_EQ, 2);
            push_simple_token(TOK_MOD);
        case '-':
            if (peek(l, 1) == '=')
                push_token(TOK_MINUS_EQ, 2);
            push_simple_token(TOK_MINUS);
        case '<':
            if (peek(l, 1) == '<' && peek(l, 2) == '=')
                push_token(TOK_LSHIFT_EQ, 3);
            if (peek(l, 1) == '<')
                push_token(TOK_LSHIFT, 2);
            if (peek(l, 1) == '=')
                push_token(TOK_LESS_EQ, 2);
            push_simple_token(TOK_LESS);
        case '>':
            if (peek(l, 1) == '>' && peek(l, 2) == '=')
                push_token(TOK_RSHIFT_EQ, 3);
            if (peek(l, 1) == '>')
                push_token(TOK_RSHIFT, 2);
            if (peek(l, 1) == '=')
                push_token(TOK_GREATER_EQ, 2);
            push_simple_token(TOK_GREATER);
        case '=':
            if (peek(l, 1) == '=')
                push_token(TOK_EQ_EQ, 2);
            push_simple_token(TOK_EQ);
        case '\'':
            u64 len = 1;
            while (!(peek(l, len) == '\'' && peek(l, len - 1) != '\\')) {
                if (peek(l, len) == '\0') {
                    CRASH("unterminated char literal");
                }
                len++;
            }
            // validity will be checked at sema-time
            add_token(l, len, TOK_CHAR);
            advance_n(l, len + 1);
            goto next_token;
        case '\"':
            len = 1;
            while (!(peek(l, len) == '\"' && peek(l, len - 1) != '\\')) {
                if (peek(l, len) == '\0') {
                    CRASH("unterminated string literal");
                }
                len++;
            }
            // validity will be checked at sema-time
            add_token(l, len, TOK_STRING);
            advance_n(l, len + 1);
            goto next_token;
        }

        if (can_begin_ident(l->current)) {
            u64 len = 1;
            while (can_ident(peek(l, len))) {
                len++;
            }
            u8 kind = categorize_keyword(&l->text.raw[l->cursor], len);
            add_token(l, len, kind);
            advance_n(l, len);
            goto next_token;
        }

        if (is_numeric(l->current)) {
            u64 len = scan_numeric(l);
            add_token(l, len, TOK_NUMERIC);
            advance_n(l, len);
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
        .current = src->text.len == 0 ? '\0' : src->text.raw[0],
    };

    tokenize(&l);
    
    return tb;
}

#define return_if_eq(kind) if (!strncmp(s, #kind, sizeof(#kind)-1)) return TOK_KEYWORD_##kind

static u8 categorize_keyword(char* s, size_t len) {
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