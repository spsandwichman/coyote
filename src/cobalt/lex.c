#include "lex.h"
#include "common/util.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static struct {
    u16 value;
    const char* spelling;
} keywords[] = {
    {TOK_KW_ALIGNAS, "alignas"},
    {TOK_KW_ALIGNAS, "_Alignas"},
    {TOK_KW_ALIGNOF, "alignof"},
    {TOK_KW_ALIGNOF, "_Alignof"},
    {TOK_KW_AUTO, "auto"},
    {TOK_KW_BOOL, "bool"},
    {TOK_KW_BOOL,"_Bool"},
    {TOK_KW_BREAK, "break"},
    {TOK_KW_CASE, "case"},
    {TOK_KW_CHAR, "char"},
    {TOK_KW_CONST, "const"},
    {TOK_KW_CONSTEXPR, "constexpr"},
    {TOK_KW_CONTINUE, "continue"},
    {TOK_KW_DEFAULT, "default"},
    {TOK_KW_DO, "do"},
    {TOK_KW_DOUBLE, "double"},
    {TOK_KW_ELSE, "else"},
    {TOK_KW_ENUM, "enum"},
    {TOK_KW_EXTERN, "extern"},
    {TOK_KW_FALSE, "false"},
    {TOK_KW_FLOAT, "float"},
    {TOK_KW_FOR, "for"},
    {TOK_KW_GOTO, "goto"},
    {TOK_KW_IF, "if"},
    {TOK_KW_INLINE, "inline"},
    {TOK_KW_INT, "int"},
    {TOK_KW_LONG, "long"},
    {TOK_KW_NULLPTR, "nullptr"},
    {TOK_KW_REGISTER, "register"},
    {TOK_KW_RESTRICT, "restrict"},
    {TOK_KW_RETURN, "return"},
    {TOK_KW_SHORT, "short"},
    {TOK_KW_SIGNED, "signed"},
    {TOK_KW_SIZEOF, "sizeof"},
    {TOK_KW_STATIC, "static"},
    {TOK_KW_STATIC_ASSERT, "static_assert"},
    {TOK_KW_STATIC_ASSERT, "_Static_assert"},
    {TOK_KW_STRUCT, "struct"},
    {TOK_KW_SWITCH, "switch"},
    {TOK_KW_THREAD_LOCAL, "thread_local"},
    {TOK_KW_THREAD_LOCAL, "_Thread_local"},
    {TOK_KW_TRUE, "true"},
    {TOK_KW_TYPEDEF, "typedef"},
    {TOK_KW_TYPEOF, "typeof"},
    {TOK_KW_TYPEOF_UNQUAL, "typeof_unqual"},
    {TOK_KW_UNION, "union"},
    {TOK_KW_UNSIGNED, "unsigned"},
    {TOK_KW_VOID, "void"},
    {TOK_KW_VOLATILE, "volatile"},
    {TOK_KW_WHILE, "while"},
    {TOK_KW_ATOMIC, "_Atomic"},
    {TOK_KW_BITINT, "_BitInt"},
    {TOK_KW_COMPLEX, "_Complex"},
    {TOK_KW_DECIMAL128, "_Decimal128"},
    {TOK_KW_DECIMAL32, "_Decimal32"},
    {TOK_KW_DECIMAL64, "_Decimal64"},
    {TOK_KW_GENERIC, "_Generic"},
    {TOK_KW_IMAGINARY, "_Imaginary"},
    {TOK_KW_NORETURN, "_Noreturn"},
};

#define countof(array) (sizeof(array) / sizeof(array[0]))

const char* token_name[] = {
    [TOK__INVALID] = "(0)",
    [TOK_EOF] = "EOF",

    [TOK_NEWLINE] = "newline",
    [TOK_IDENT]   = "identifier",

    [TOK_OPEN_PAREN]    = "(",
    [TOK_CLOSE_PAREN]   = ")",
    [TOK_OPEN_BRACE]    = "{",
    [TOK_CLOSE_BRACE]   = "}",
    [TOK_OPEN_BRACKET]  = "[",
    [TOK_CLOSE_BRACKET] = "]",
    
    [TOK_SEMICOLON] = ";",
    [TOK_COLON]     = ":",
    [TOK_COMMA]     = ",",
    [TOK_DOT]       = ".",
    [TOK_TILDE]     = "~",
    [TOK_ARROW]     = "->",
    [TOK_ELLIPSIS]  = "...",

    [TOK_EQ]     = "=",
    [TOK_ADD_EQ] = "+=",
    [TOK_SUB_EQ] = "-=",
    [TOK_MUL_EQ] = "*=",
    [TOK_DIV_EQ] = "/=",
    [TOK_REM_EQ] = "%=",
    [TOK_AND_EQ] = "&=",
    [TOK_OR_EQ]  = "|=",
    [TOK_XOR_EQ] = "^=",
    [TOK_LSH_EQ] = "<<=",
    [TOK_RSH_EQ] = ">>=",

    [TOK_ADD] = "+",
    [TOK_SUB] = "-",
    [TOK_MUL] = "*",
    [TOK_DIV] = "/",
    [TOK_REM] = "%",
    [TOK_AND] = "&",
    [TOK_OR]  = "|",
    [TOK_XOR] = "^",
    [TOK_LSH] = "<<",
    [TOK_RSH] = ">>",

    [TOK_EXCLAM]     = "!",
    [TOK_AND_AND]    = "&&",
    [TOK_OR_OR]      = "||",

    [TOK_EQ_EQ]      = "==",
    [TOK_NOT_EQ]     = "!=",
    [TOK_LESS_EQ]    = "<=",
    [TOK_GREATER_EQ] = ">=",
    [TOK_LESS]       = "<",
    [TOK_GREATER]    = ">",

    [TOK_HASH]       = "#",
    [TOK_HASH_HASH]  = "##",

    [TOK_KW_ALIGNAS] = "alignas",
    [TOK_KW_ALIGNOF] = "alignof",
    [TOK_KW_AUTO] = "auto",
    [TOK_KW_BOOL] = "bool",
    [TOK_KW_BREAK] = "break",
    [TOK_KW_CASE] = "case",
    [TOK_KW_CHAR] = "char",
    [TOK_KW_CONST] = "const",
    [TOK_KW_CONSTEXPR] = "constexpr",
    [TOK_KW_CONTINUE] = "continue",
    [TOK_KW_DEFAULT] = "default",
    [TOK_KW_DO] = "do",
    [TOK_KW_DOUBLE] = "double",
    [TOK_KW_ELSE] = "else",
    [TOK_KW_ENUM] = "enum",
    [TOK_KW_EXTERN] = "extern",
    [TOK_KW_FALSE] = "false",
    [TOK_KW_FLOAT] = "float",
    [TOK_KW_FOR] = "for",
    [TOK_KW_GOTO] = "goto",
    [TOK_KW_IF] = "if",
    [TOK_KW_INLINE] = "inline",
    [TOK_KW_INT] = "int",
    [TOK_KW_LONG] = "long",
    [TOK_KW_NULLPTR] = "nullptr",
    [TOK_KW_REGISTER] = "register",
    [TOK_KW_RESTRICT] = "restrict",
    [TOK_KW_RETURN] = "return",
    [TOK_KW_SHORT] = "short",
    [TOK_KW_SIGNED] = "signed",
    [TOK_KW_SIZEOF] = "sizeof",
    [TOK_KW_STATIC] = "static",
    [TOK_KW_STATIC_ASSERT] = "static_assert",
    [TOK_KW_STRUCT] = "struct",
    [TOK_KW_SWITCH] = "switch",
    [TOK_KW_THREAD_LOCAL] = "thread_local",
    [TOK_KW_TRUE] = "true",
    [TOK_KW_TYPEDEF] = "typedef",
    [TOK_KW_TYPEOF] = "typeof",
    [TOK_KW_TYPEOF_UNQUAL] = "typeof_unqual",
    [TOK_KW_UNION] = "union",
    [TOK_KW_UNSIGNED] = "unsigned",
    [TOK_KW_VOID] = "void",
    [TOK_KW_VOLATILE] = "volatile",
    [TOK_KW_WHILE] = "while",
    [TOK_KW_ATOMIC] = "_Atomic",
    [TOK_KW_BITINT] = "_BitInt",
    [TOK_KW_COMPLEX] = "_Complex",
    [TOK_KW_DECIMAL128] = "_Decimal128",
    [TOK_KW_DECIMAL64] = "_Decimal64",
    [TOK_KW_DECIMAL32] = "_Decimal32",
    [TOK_KW_GENERIC] = "_Generic",
    [TOK_KW_IMAGINARY] = "_Imaginary",
    [TOK_KW_NORETURN] = "_Noreturn",
};

// // FAST LOOKUP BAYBEE
// static bool token_is_valid[UINT8_MAX + UINT8_MAX * 2 + UINT8_MAX * 4] = {

// }

static bool is_alphabetic(char c) {
    return 
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        c == '_';
}
static bool is_alphanumeric(char c) {
    return 
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9') ||
        c == '_';
}
static bool is_numeric(char c) {
    return ('0' <= c && c <= '9');
}
static bool is_whitespace(char c) {
    switch (c) {
    // case '\n':
    case '\0': case ' ': case '\t': case '\r': case '\v':
        return true;
    default:
        return false;
    }
}

Token lex_next_raw(Lexer* l) {
    while (true) {
        if_unlikely (l->cursor > l->src.len) {
            return (Token){
                .generated = false,
                .kind = TOK_EOF,
                .len = 1,
                .raw = &l->src.raw[l->src.len - 1],
            };
        }

        if (!is_whitespace(l->src.raw[l->cursor])) {
            break;
        }
        l->cursor += 1;
    }

    Token t = {
        .generated = false,
        .raw = &l->src.raw[l->cursor],
    };

    UNREACHABLE;
    
    l->cursor += t.len;
    return t;
}
