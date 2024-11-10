#include "front.h"

typedef struct Parser {
    ParseTree tree;

    TokenBuf tb;
    usize cursor;
} Parser;
static Parser p;

// dynamic array of indices that the parser shares across functions
static struct {
    Index* at;
    u32 len;
    u32 cap;
    u32 start;
} dynbuf;

static u32 save_dynbuf() {
    da_append(&dynbuf, dynbuf.start);
    dynbuf.start = dynbuf.len;
    return dynbuf.len - 1;
}

static void restore_dynbuf(u32 save) {
    dynbuf.len = save;
    dynbuf.start = dynbuf.at[save];
}

// BEWARE: this may invalidate any active ParseNode*
Index new_node(u8 kind) {
    if (p.tree.nodes.cap == p.tree.nodes.cap) {
        p.tree.nodes.cap *= 2;
        p.tree.nodes.at = realloc(p.tree.nodes.at, p.tree.nodes.cap);
        p.tree.nodes.kinds = realloc(p.tree.nodes.kinds, p.tree.nodes.cap);
    }

    Index index = p.tree.nodes.len++;
    p.tree.nodes.kinds[index] = kind;
    p.tree.nodes.at[index] = (ParseNode){0};
    p.tree.nodes.at[index].main_token = p.cursor;

    return index;
}

Index new_extra_slots(usize slots) {
    if (p.tree.extra.cap == p.tree.extra.cap) {
        p.tree.extra.cap *= 2;
        p.tree.extra.at = realloc(p.tree.extra.at, p.tree.extra.cap);
    }

    Index index = p.tree.extra.len += slots;
    memset(&p.tree.extra.at[index], 0, slots * sizeof(p.tree.extra.at[0]));

    return index;
}

// this pointer becomes invalid
static inline ParseNode* node(Index i) {
    return &p.tree.nodes.at[i];
}

static inline u8 kind(Index i) {
    return p.tree.nodes.kinds[i];
}

static inline void set_kind(Index i, u8 kind) {
   p.tree.nodes.kinds[i] = kind;
}

static inline Token* peek(isize n) {
    if (p.cursor + n >= p.tb.len) {
        return &p.tb.at[p.tb.len - 1];
    }
    return &p.tb.at[p.cursor + n];
}

static inline Token* current() {
    return &p.tb.at[p.cursor];
}

static inline void advance() {
    p.cursor++;
}

static void report_token(bool error, u32 token_index, char* message, ...) {
    va_list varargs;
    va_start(varargs, message);
    Token t = p.tb.at[token_index];
    string t_text = {t.raw, t.len};
    bool found = false;
    for_range(i, 0, ctx.sources.len) {
        SourceFile srcfile = ctx.sources.at[i];
        if (is_within(srcfile.text, t_text)) {
            emit_report(error, srcfile.text, srcfile.path, t_text, message, varargs);
            found = true;
        }
    }

    if (!found) CRASH("unable to locate source file of token");
}

static void expect(u8 kind) {
    if (current()->kind != kind) {
        report_token(true, p.cursor, "expected %s, got %s", token_kind[kind], token_kind[current()->kind]);
    }
}

#define new_extra(T) new_extra_slots(sizeof(T) / sizeof(Index))
#define new_extra_with(T, n) new_extra_slots(sizeof(T) / sizeof(Index) + n)

#define as_extra(T, index) ((T*)&p.tree.extra.at[index])

// returns index to PNExtraFnProto
Index parse_fn_prototype() {

    Index fnptr_type = NULL_INDEX;
    Index ident;

    // consume provided fnptr?
    if (current()->kind == TOK_OPEN_PAREN) {
        advance();
        fnptr_type = parse_type();
        expect(TOK_CLOSE_PAREN);
        advance();
    }

    // function identifier
    expect(TOK_IDENTIFIER);
    ident = p.cursor;
    advance();

    // use the dynbuf to parse params
    u32 dbsave = save_dynbuf();

    expect(TOK_OPEN_PAREN);
    advance();
    while (current()->kind != TOK_CLOSE_PAREN) {
        // (IN | OUT) ident : type,
        u8 pkind;
        if (current()->kind == TOK_KEYWORD_IN) {
            pkind == PN_IN_PARAM;
        } else if (current()->kind == TOK_KEYWORD_OUT) {
            pkind == PN_OUT_PARAM;
        } else {
            report_token(true, p.cursor, "expected IN or OUT");
        }
        advance();

        Index param = new_node(pkind);
        expect(TOK_IDENTIFIER);
        node(param)->bin.lhs = p.cursor; // mark identifier index
        advance();

        expect(TOK_COLON); // :
        advance();

        // type
        node(param)->bin.rhs = parse_type();

        da_append(&dynbuf, param);

        if (current()->kind != TOK_COMMA) {
            break;
        } else {
            advance();
        }
    }
    expect(TOK_CLOSE_PAREN);
    advance();

    // copy into fn prototype
    Index proto_index = new_extra_with(PNExtraFnProto, dynbuf.len - dynbuf.start);
    as_extra(PNExtraFnProto, proto_index)->fnptr_type = fnptr_type;
    as_extra(PNExtraFnProto, proto_index)->identifier = ident;
    as_extra(PNExtraFnProto, proto_index)->params_len = dynbuf.len;
    for_range(i, dynbuf.start, dynbuf.len) {
        as_extra(PNExtraFnProto, proto_index)->params[i] = dynbuf.at[i];
    }

    if (current()->kind == TOK_COLON) {
        advance();
        as_extra(PNExtraFnProto, proto_index)->return_type = parse_type();
    }

    restore_dynbuf(dbsave);
}

static u8 decl_node_kind(u8 storage, bool fn) {
    if (fn) {
        switch (storage) {
        case TOK_KEYWORD_PRIVATE: return PN_STMT_PRIVATE_FN_DECL;
        case TOK_KEYWORD_PUBLIC: return PN_STMT_PUBLIC_FN_DECL;
        case TOK_KEYWORD_EXPORT: return PN_STMT_EXPORT_FN_DECL;
        }
    } else {
        switch (storage) {
        case TOK_KEYWORD_PRIVATE: return PN_STMT_PRIVATE_DECL;
        case TOK_KEYWORD_PUBLIC: return PN_STMT_PUBLIC_DECL;
        case TOK_KEYWORD_EXPORT: return PN_STMT_EXPORT_DECL;
        }
    }
    return 0;
}

Index parse_var_decl(bool is_extern) {
    TODO("");
}

static bool is_storage_class(u8 kind) {
    switch (kind) {
    case TOK_KEYWORD_PRIVATE:
    case TOK_KEYWORD_PUBLIC:
    case TOK_KEYWORD_EXPORT:
        return true;
    }
    return false;
}

// this function is fucked lmao, redo this
Index parse_decl() {
    if (current()->kind == TOK_KEYWORD_EXTERN) {
        advance();
        if (current()->kind == TOK_KEYWORD_FN) {
            Index extern_fn_index = new_node(PN_STMT_EXTERN_FN);
            advance();
            node(extern_fn_index)->bin.lhs = parse_fn_prototype();
            return extern_fn_index;
        }

        // else, EXTERN ident : type
        if (current()->kind != TOK_IDENTIFIER) {
            report_token(true, p.cursor, "expected FN or identifier");
        }

        Index decl_index = parse_var_decl(true);
        set_kind(decl_index, PN_STMT_EXTERN_DECL);
        return decl_index;
    }
    
    u8 decl_kind = 0;
    if (is_storage_class(current()->kind)) {
        decl_kind = decl_node_kind(current()->kind, peek(1)->kind == TOK_KEYWORD_FN);
        advance();
    }
    
    Index decl_index;
    if (current()->kind == TOK_KEYWORD_FN) {
        decl_index = parse_fn_decl();
    } else if (current()->kind == TOK_IDENTIFIER) {
        decl_index = parse_var_decl(false);
    } else {
        report_token(true, p.cursor, "expected a declaration");
    }
    if (decl_kind) set_kind(decl_index, decl_kind);
    return decl_index;
}

Index parse_fn_decl() {
    expect(TOK_KEYWORD_FN);
    Index fn_decl = new_node(PN_STMT_FN_DECL);
    advance();
    node(fn_decl)->bin.lhs = parse_fn_prototype();

    // use the dynbuf to parse params
    u32 sp = save_dynbuf();

    while (current()->kind != TOK_KEYWORD_END) {
        Index stmt = parse_stmt();
        da_append(&dynbuf, stmt);
    }

    Index stmt_list = new_extra_with(PNExtraList, dynbuf.len - dynbuf.start);
    as_extra(PNExtraList, stmt_list)->len = dynbuf.len - dynbuf.start;
    for_range(i, dynbuf.start, dynbuf.len) {
        as_extra(PNExtraList, stmt_list)->items[i - dynbuf.start] = dynbuf.at[i];
    }
    node(fn_decl)->bin.rhs = stmt_list;

    restore_dynbuf(sp);

    return fn_decl;
}

Index parse_structure_decl(bool is_struct) {
    Index decl_index = new_node(is_struct ? PN_STMT_STRUCT_DECL : PN_STMT_UNION_DECL);
    node(decl_index)->main_token = p.cursor;
    advance();
    if (!is_struct && current()->kind == TOK_KEYWORD_PACKED) {
        p.tree.nodes.kinds[decl_index] = PN_STMT_STRUCT_PACKED_DECL;
        advance();
    }
    expect(TOK_IDENTIFIER);
    TODO("");
}

Index parse_enum_decl() {
    TODO("");
}

Index parse_type_decl() {
    TODO("");
}

Index parse_stmt() {
    switch (current()->kind) {
    case TOK_KEYWORD_NOTHING:
        advance();
        return parse_stmt();
    case TOK_KEYWORD_PROBE: u8 simple_kind = PN_STMT_PROBE; goto simple;
    case TOK_KEYWORD_BARRIER:  simple_kind = PN_STMT_BARRIER; goto simple;
    case TOK_KEYWORD_BREAK:    simple_kind = PN_STMT_BREAK; goto simple;
    case TOK_KEYWORD_CONTINUE: simple_kind = PN_STMT_CONTINUE; goto simple;
    case TOK_KEYWORD_LEAVE:    simple_kind = PN_STMT_LEAVE;
        simple:
        Index simple_index = new_node(simple_kind);
        advance();
        return simple_index;
    case TOK_KEYWORD_RETURN:
        Index ret_stmt = new_node(PN_STMT_RETURN);
        advance();
        node(ret_stmt)->bin.lhs = parse_expr();
        return ret_stmt;
    case TOK_KEYWORD_GOTO:
    case TOK_AT:
        Index jump_stmt = new_node(current()->kind == TOK_AT ? PN_STMT_LABEL : PN_STMT_GOTO);
        advance();
        expect(TOK_IDENTIFIER);
        advance();
        return jump_stmt;
    case TOK_KEYWORD_PUBLIC:
    case TOK_KEYWORD_PRIVATE:
    case TOK_KEYWORD_EXPORT:
    case TOK_KEYWORD_EXTERN:
        return parse_decl();
    case TOK_KEYWORD_FN:
        return parse_fn_decl();
    case TOK_KEYWORD_STRUCT: 
    case TOK_KEYWORD_UNION:
        return parse_structure_decl(current()->kind == TOK_KEYWORD_STRUCT);
    case TOK_KEYWORD_ENUM:
        return parse_enum_decl();
    case TOK_KEYWORD_TYPE:
        return parse_type_decl();
    default:
        report_token(true, p.cursor, "expected a statement");
    }
}

Index parse_type() {
    Index t = parse_base_type();
    while (current()->kind == TOK_OPEN_BRACKET) {
        Index new = new_node(PN_TYPE_ARRAY);
        advance();
        node(new)->bin.lhs = t;
        node(new)->bin.rhs = parse_expr();
        expect(TOK_CLOSE_BRACKET);
        t = new;
    }
    return t;
}

Index parse_base_type() {
    switch (current()->kind){
    case TOK_CARET:
        advance();
        Index ptr = new_node(PN_TYPE_POINTER);
        node(ptr)->bin.lhs = parse_base_type();
        return ptr;
    case TOK_OPEN_PAREN:
        advance();
        Index i = parse_type();
        expect(TOK_CLOSE_PAREN);
        advance();
        return i;
    case TOK_KEYWORD_QUAD: u8 simple_kind = PN_TYPE_QUAD; goto simple_type;
    case TOK_KEYWORD_UQUAD:   simple_kind = PN_TYPE_UQUAD; goto simple_type;
    case TOK_KEYWORD_LONG:    simple_kind = PN_TYPE_LONG; goto simple_type;
    case TOK_KEYWORD_ULONG:   simple_kind = PN_TYPE_ULONG; goto simple_type;
    case TOK_KEYWORD_INT:     simple_kind = PN_TYPE_INT; goto simple_type;
    case TOK_KEYWORD_UINT:    simple_kind = PN_TYPE_UINT; goto simple_type;
    case TOK_KEYWORD_BYTE:    simple_kind = PN_TYPE_BYTE; goto simple_type;
    case TOK_KEYWORD_UBYTE:   simple_kind = PN_TYPE_UBYTE; goto simple_type;
    case TOK_KEYWORD_VOID:    simple_kind = PN_TYPE_VOID;
        simple_type:
        Index simple_index = new_node(simple_kind);
        advance();
        return simple_index;
    default:
        report_token(true, p.cursor, "expected a type");
        break;
    }
}

Index parse_selector_sequence() {
    TODO("");
}

Index parse_atomic_terminal() {
    TODO("");
    
}

Index parse_atomic() {
    Index expr = parse_atomic_terminal();
}

Index parse_unary() {
    switch (current()->kind){
    case TOK_KEYWORD_CAST:
        Index cast = new_node(PN_EXPR_CAST);
        advance();
        node(cast)->bin.lhs = parse_expr();
        expect(TOK_KEYWORD_TO);
        advance();
        node(cast)->bin.rhs = parse_type();
        return cast;
    case TOK_KEYWORD_CONTAINEROF:
        TODO("containerof");
    case TOK_AND: u8 simple_kind = PN_EXPR_ADDRESS; goto simple_op;
    case TOK_MINUS: simple_kind = PN_EXPR_NEGATE; goto simple_op;
    case TOK_TILDE: simple_kind = PN_EXPR_BIT_NOT; goto simple_op;
    case TOK_KEYWORD_NOT: simple_kind = PN_EXPR_BOOL_NOT; goto simple_op;
    case TOK_KEYWORD_SIZEOFVALUE: simple_kind = PN_EXPR_SIZEOFVALUE; goto simple_op;
        simple_op:
        Index simple_index = new_node(simple_kind);
        advance();
        node(simple_index)->bin.lhs = parse_unary();
        return simple_index;
    default:
        return parse_atomic();
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
        case TOK_DOLLAR:
            return 6;
        case TOK_OR:
            return 5;
        case TOK_LESS:
        case TOK_LESS_EQ:
        case TOK_GREATER:
        case TOK_GREATER_EQ:
            return 4;
        case TOK_EQ:
        case TOK_NOT_EQ:
            return 3;
        case TOK_KEYWORD_AND:
            return 2;
        case TOK_KEYWORD_OR:
            return 1;
    }
    return -1;
}

Index parse_binary_recurse(Index lhs, isize precedence) {

}

Index parse_binary(isize precedence) {
    Index lhs = parse_unary();

    // while (precedence < bin-prec)
    TODO("");
}

Index parse_expr() {
    return parse_binary(0);
}

ParseTree parse_file(TokenBuf tb) {
    p.tb = tb;
    p.cursor = 0;
    p.tree = (ParseTree){0};
    p.tree.head = 1;

    p.tree.nodes.len = 0;
    p.tree.nodes.cap = 128;
    p.tree.nodes.at = malloc(sizeof(p.tree.nodes.at[0]) * p.tree.nodes.cap);

    p.tree.extra.len = 0;
    p.tree.extra.cap = 128;
    p.tree.extra.at = malloc(sizeof(p.tree.extra.at[0]) * p.tree.extra.cap);

    // initialize dynbuf
    da_init(&dynbuf, 32);

    // occupy null node
    if (new_node(0) != 0) {
        CRASH("null node is not at index 0");
    }

    while (current()->kind != TOK_EOF) {
        Index decl = parse_decl();
    }
}