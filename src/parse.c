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
    Index index = p.tree.nodes.len++;
    if (p.tree.nodes.len == p.tree.nodes.cap) {
        p.tree.nodes.cap *= 2;
        p.tree.nodes.at = realloc(p.tree.nodes.at, sizeof(p.tree.nodes.at[0]) * p.tree.nodes.cap);
        p.tree.nodes.kinds = realloc(p.tree.nodes.kinds, sizeof(p.tree.nodes.kinds[0]) * p.tree.nodes.cap);
        assert(p.tree.nodes.at != NULL);
        assert(p.tree.nodes.kinds != NULL);
    }

    p.tree.nodes.kinds[index] = kind;
    p.tree.nodes.at[index] = (ParseNode){0};
    p.tree.nodes.at[index].main_token = p.cursor;

    return index;
}

Index new_extra_slots(usize slots) {
    Index index = p.tree.extra.len;
    p.tree.extra.len += slots;
    if (p.tree.extra.len >= p.tree.extra.cap) {
        p.tree.extra.cap *= 2;
        p.tree.extra.cap += slots;
        p.tree.extra.at = realloc(p.tree.extra.at, sizeof(p.tree.extra.at[0]) * p.tree.extra.cap);
        assert(p.tree.extra.at);
    }

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

static void expect(u8 kind) {
    if (current()->kind != kind) {
        report_token(true, p.cursor, "expected %s, got %s", token_kind[kind], token_kind[current()->kind]);
    }
}

#define new_extra(T) new_extra_slots(sizeof(T) / sizeof(Index))
#define new_extra_with(T, n) new_extra_slots(sizeof(T) / sizeof(Index) + n)

#define as_extra(T, index) ((T*)&p.tree.extra.at[index])

// returns index to PNExtraFnProto
Index parse_fn_prototype(bool is_fnptr) {

    Index fnptr_type = NULL_INDEX;
    Index ident;

    // consume provided fnptr?
    if (current()->kind == TOK_OPEN_PAREN) {
        if (is_fnptr) {
            report_token(true, p.cursor, "FNPTR declaration cannot have a type");
        }
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
        // ... argv argc,
        if (current()->kind == TOK_VARARG) {
            Index vararg = new_node(PN_VAR_PARAM);
            advance();
            expect(TOK_IDENTIFIER);
            node(vararg)->lhs = p.cursor; // argv
            advance();
            expect(TOK_IDENTIFIER);
            node(vararg)->rhs = p.cursor; // argc
            advance();

            if (current()->kind == TOK_COMMA) {
                advance();
            }
            da_append(&dynbuf, vararg);
            break;
        }

        // (IN | OUT) ident : type,
        u8 pkind = 0;
        if (current()->kind == TOK_KEYWORD_IN) {
            pkind = PN_IN_PARAM;
        } else if (current()->kind == TOK_KEYWORD_OUT) {
            pkind = PN_OUT_PARAM;
        } else {
            report_token(true, p.cursor, "expected IN or OUT");
        }
        advance();

        Index param = new_node(pkind);
        expect(TOK_IDENTIFIER);
        node(param)->lhs = p.cursor; // mark identifier index
        advance();

        expect(TOK_COLON); // :
        advance();

        // type
        Index type = parse_type();
        node(param)->rhs = type;

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
        as_extra(PNExtraFnProto, proto_index)->params[i - dynbuf.start] = dynbuf.at[i];
    }

    if (current()->kind == TOK_COLON) {
        advance();
        Index type = parse_type();
        as_extra(PNExtraFnProto, proto_index)->return_type = type;
    }
    restore_dynbuf(dbsave);
    return proto_index;
}

Index parse_compound_expr() {
    Index compound = new_node(PN_COMPOUND_EXPR);
    advance();

    u32 sp = save_dynbuf();

    while (current()->kind != TOK_CLOSE_BRACE) {
        if (current()->kind == TOK_OPEN_BRACKET) {
            // [expr] = expr,
            Index item = new_node(PN_COMPOUND_ITEM);
            advance();
            Index indexer = parse_expr();
            node(item)->lhs = indexer;
            expect(TOK_CLOSE_BRACKET);
            advance();
            expect(TOK_EQ);
            advance();
            Index value = parse_expr();
            node(item)->rhs = value;
            da_append(&dynbuf, item);
        } else {
            // expr,
            Index item = parse_expr();
            da_append(&dynbuf, item);
        }

        if (current()->kind == TOK_COMMA) {
            advance();
        } else {
            break;
        }
    }
    expect(TOK_CLOSE_BRACE);
    advance();

    Index list = new_extra_with(PNExtraList, dynbuf.len - dynbuf.start);
    as_extra(PNExtraList, list)->len = dynbuf.len - dynbuf.start;
    for_range(i, dynbuf.start, dynbuf.len) {
        as_extra(PNExtraList, list)->items[i - dynbuf.start] = dynbuf.at[i];
    }
    node(compound)->lhs = list;

    restore_dynbuf(sp);

    return compound;
}

Index parse_var_decl(bool is_extern) {
    Index decl = new_node(PN_STMT_DECL);
    expect(TOK_IDENTIFIER);
    advance();
    expect(TOK_COLON);
    advance();
    if (current()->kind != TOK_EQ) {
        Index type = parse_type();
        node(decl)->lhs = type;
    } else if (is_extern) {
        report_token(true, p.cursor, "EXTERN decls must have a type");
    }

    if (current()->kind == TOK_EQ) {
        if (is_extern) {
            report_token(true, p.cursor, "EXTERN decls cannot have a value");
        }
        advance();
        if (current()->kind == TOK_OPEN_BRACE) {
            Index val = parse_compound_expr();
            node(decl)->rhs = val;
        } else {
            Index val = parse_expr();
            node(decl)->rhs = val;
        }
    }
    return decl;
}

// this function is kinda fucked lmao, its good enough
Index parse_decl() {
    if (current()->kind == TOK_KEYWORD_EXTERN) {
        advance();
        if (current()->kind == TOK_KEYWORD_FN) {
            Index extern_fn_index = new_node(PN_STMT_EXTERN_FN_DECL);
            advance();
            Index prototype = parse_fn_prototype(false);
            node(extern_fn_index)->lhs = prototype;
            return extern_fn_index;
        }

        // EXTERN ident : type
        Index decl_index = parse_var_decl(true);
        set_kind(decl_index, PN_STMT_EXTERN_DECL);
        return decl_index;
    }
    
    u8 decl_kind = 0;
    switch (current()->kind) {
    case TOK_KEYWORD_PUBLIC: advance(); decl_kind = PN_STMT_PUBLIC_DECL; break;
    case TOK_KEYWORD_EXPORT: advance(); decl_kind = PN_STMT_EXPORT_DECL; break;
    case TOK_KEYWORD_PRIVATE: advance(); decl_kind = PN_STMT_PRIVATE_DECL; break;
    case TOK_IDENTIFIER: decl_kind = PN_STMT_DECL; break;
    }

    if (current()->kind == TOK_KEYWORD_FN) {
        if (decl_kind != 0) decl_kind += PN_STMT_FN_DECL - PN_STMT_DECL;
        Index fn = parse_fn_decl();
        set_kind(fn, decl_kind);
        return fn;
    }

    Index decl = parse_var_decl(false);
    if (decl_kind) set_kind(decl, decl_kind);
    return decl;
}

Index parse_stmt_list(bool if_stmt) {
    u32 sp = save_dynbuf();
    for (u8 k = current()->kind; k != TOK_KEYWORD_END; k = current()->kind) {
        if (if_stmt && (k == TOK_KEYWORD_ELSE || k == TOK_KEYWORD_ELSEIF)) {
            p.cursor--;
            break;
        }
        Index stmt = parse_stmt();
        da_append(&dynbuf, stmt);
    }
    advance();
    Index stmt_list = new_extra_with(PNExtraList, dynbuf.len - dynbuf.start);
    as_extra(PNExtraList, stmt_list)->len = dynbuf.len - dynbuf.start;
    for_range(i, dynbuf.start, dynbuf.len) {
        as_extra(PNExtraList, stmt_list)->items[i - dynbuf.start] = dynbuf.at[i];
    }
    restore_dynbuf(sp);
    return stmt_list;
}

Index parse_fn_decl() {
    expect(TOK_KEYWORD_FN);
    Index fn_decl = new_node(PN_STMT_FN_DECL);
    advance();
    Index prototype = parse_fn_prototype(false);
    node(fn_decl)->lhs = prototype;
    Index stmt_list = parse_stmt_list(false);
    node(fn_decl)->rhs = stmt_list;
    return fn_decl;
}

Index parse_structure_decl(bool is_struct) {
    Index decl = new_node(is_struct ? PN_STMT_STRUCT_DECL : PN_STMT_UNION_DECL);
    node(decl)->main_token = p.cursor;
    advance();
    if (!is_struct && current()->kind == TOK_KEYWORD_PACKED) {
        set_kind(decl, PN_STMT_STRUCT_PACKED_DECL);
        advance();
    }
    expect(TOK_IDENTIFIER);
    advance();

    u32 sp = save_dynbuf();

    while (current()->kind != TOK_KEYWORD_END) {
        expect(TOK_IDENTIFIER);
        Index field = new_node(PN_FIELD);
        node(field)->lhs = p.cursor;
        advance();
        expect(TOK_COLON);
        advance();
        Index type = parse_type();
        node(field)->rhs = type;
        da_append(&dynbuf, field);

        if (current()->kind == TOK_COMMA) {
            advance();
        } else {
            break;
        }
    }
    expect(TOK_KEYWORD_END);
    advance();

    // copy into hard buffer
    Index list = new_extra_with(PNExtraList, dynbuf.len - dynbuf.start);
    as_extra(PNExtraList, list)->len = dynbuf.len - dynbuf.start;
    for_range(i, dynbuf.start, dynbuf.len) {
        as_extra(PNExtraList, list)->items[i - dynbuf.start] = dynbuf.at[i];
    }

    node(decl)->rhs = list;

    restore_dynbuf(sp);
    return decl;
}

Index parse_enum_decl() {
    Index decl = new_node(PN_STMT_ENUM_DECL);
    advance();
    expect(TOK_IDENTIFIER);
    advance();
    expect(TOK_COLON);
    advance();
    Index type = parse_type();
    node(decl)->lhs = type;

    u32 sp = save_dynbuf();

    while (current()->kind != TOK_KEYWORD_END) {
        expect(TOK_IDENTIFIER);
        Index variant = new_node(PN_VARIANT);
        node(variant)->lhs = p.cursor;
        advance();
        if (current()->kind == TOK_EQ) {
            advance();
            Index value = parse_expr();
            node(variant)->rhs = value;
        }
        da_append(&dynbuf, variant);

        if (current()->kind == TOK_COMMA) {
            advance();
        } else {
            break;
        }
    }
    expect(TOK_KEYWORD_END);
    advance();

    // copy into hard buffer
    Index list = new_extra_with(PNExtraList, dynbuf.len - dynbuf.start);
    as_extra(PNExtraList, list)->len = dynbuf.len - dynbuf.start;
    for_range(i, dynbuf.start, dynbuf.len) {
        as_extra(PNExtraList, list)->items[i - dynbuf.start] = dynbuf.at[i];
    }
    node(decl)->rhs = list;

    restore_dynbuf(sp);
    return decl;
}

Index parse_type_decl() {
    Index decl = new_node(PN_STMT_TYPE_DECL);
    advance();
    expect(TOK_IDENTIFIER);
    node(decl)->lhs = p.cursor;
    advance();
    expect(TOK_COLON);
    advance();
    Index type = parse_type();
    node(decl)->rhs = type;
}

Index parse_fnptr_decl() {
    Index i = new_node(PN_STMT_FNPTR_DECL);
    advance();
    Index proto = parse_fn_prototype(true);
    node(i)->lhs = proto;
    return i;
}

u8 assignop(u8 token_kind) {
    if (TOK_EQ <= token_kind && token_kind <= TOK_RSHIFT_EQ) {
        return token_kind - TOK_EQ + PN_STMT_ASSIGN;
    }
    return 0;
}

Index parse_if_stmt() {
    Index if_stmt = new_node(PN_STMT_IF);
    advance();
    Index cond = parse_expr();
    node(if_stmt)->lhs = cond;
    Index true_branch = parse_stmt_list(true);
    if (current()->kind == TOK_KEYWORD_ELSE) {
        advance();
        Index else_extra = new_extra(PNExtraIfElseStmt);
        Index false_branch = parse_stmt_list(true);
        as_extra(PNExtraIfElseStmt, else_extra)->block = true_branch;
        as_extra(PNExtraIfElseStmt, else_extra)->else_branch = false_branch;
        node(if_stmt)->rhs = else_extra;
    } else if (current()->kind == TOK_KEYWORD_ELSEIF) {
        Index else_extra = new_extra(PNExtraIfElseStmt);
        as_extra(PNExtraIfElseStmt, else_extra)->block = true_branch;
        Index elseif = parse_if_stmt();
        as_extra(PNExtraIfElseStmt, else_extra)->else_branch = elseif;
    } else {
        node(if_stmt)->rhs = true_branch;
    }
    return if_stmt;
}

Index parse_stmt() {
    switch (current()->kind) {
    case TOK_KEYWORD_NOTHING:
        advance();
        return parse_stmt();
    case TOK_KEYWORD_BARRIER:u8 simple_kind = PN_STMT_BARRIER; goto simple;
    case TOK_KEYWORD_BREAK:     simple_kind = PN_STMT_BREAK; goto simple;
    case TOK_KEYWORD_CONTINUE:  simple_kind = PN_STMT_CONTINUE; goto simple;
    case TOK_KEYWORD_LEAVE:     simple_kind = PN_STMT_LEAVE;
        simple:
        Index simple_index = new_node(simple_kind);
        advance();
        return simple_index;
    case TOK_KEYWORD_RETURN:
        Index ret_stmt = new_node(PN_STMT_RETURN);
        advance();
        Index ret_expr = parse_expr();
        node(ret_stmt)->lhs = ret_expr;
        return ret_stmt;
    case TOK_KEYWORD_GOTO:
    case TOK_AT:
        Index jump_stmt = new_node(current()->kind == TOK_AT ? PN_STMT_LABEL : PN_STMT_GOTO);
        advance();
        expect(TOK_IDENTIFIER);
        node(jump_stmt)->lhs = p.cursor;
        advance();
        return jump_stmt;
    case TOK_IDENTIFIER:
        if (peek(1)->kind == TOK_COLON) {
            return parse_var_decl(false);
        } else {
            Index lhs = parse_expr();
            // this could be an assignment, or just a plain expr
            u8 assign_kind = assignop(current()->kind);
            if (assign_kind) {
                Index assign = new_node(assign_kind);
                advance();
                node(assign)->lhs = lhs;
                Index rhs = parse_expr();
                node(assign)->rhs = rhs;
                return assign;
            } else {
                return lhs;
            }
        }
    case TOK_KEYWORD_WHILE:
        Index while_stmt = new_node(PN_STMT_WHILE);
        advance();
        Index cond = parse_expr();
        node(while_stmt)->lhs = cond;
        Index stmt_list = parse_stmt_list(false);
        node(while_stmt)->rhs = stmt_list;
        return while_stmt;
    case TOK_KEYWORD_IF:
        
        return parse_if_stmt();
    case TOK_KEYWORD_PUBLIC:
    case TOK_KEYWORD_PRIVATE:
    case TOK_KEYWORD_EXPORT:
    case TOK_KEYWORD_EXTERN:
        return parse_decl();
    case TOK_KEYWORD_FN:
        return parse_fn_decl();
    case TOK_KEYWORD_STRUCT: 
        return parse_structure_decl(false);
    case TOK_KEYWORD_UNION:
        return parse_structure_decl(true);
    case TOK_KEYWORD_ENUM:
        return parse_enum_decl();
    case TOK_KEYWORD_TYPE:
        return parse_type_decl();
    case TOK_KEYWORD_FNPTR:
        return parse_fnptr_decl();
    default:
        report_token(true, p.cursor, "expected a statement");
    }
    return 0;
}

Index parse_type() {
    Index t = parse_base_type();
    while (current()->kind == TOK_OPEN_BRACKET) {
        Index new = new_node(PN_TYPE_ARRAY);
        advance();
        node(new)->lhs = t;
        Index expr = parse_expr();
        node(new)->rhs = expr;
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
        Index base_type = parse_base_type();
        node(ptr)->lhs = base_type;
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
    case TOK_IDENTIFIER:
        Index ident = new_node(PN_EXPR_IDENT);
        advance();
        return ident;
    default:
        report_token(true, p.cursor, "expected a type");
        break;
    }
    return 0;
}

Index parse_atomic_terminal() {
    switch (current()->kind) {
    case TOK_KEYWORD_TRUE:
        Index bool_true = new_node(PN_EXPR_BOOL_TRUE);
        advance();
        return bool_true;
    case TOK_KEYWORD_FALSE:
        Index bool_false = new_node(PN_EXPR_BOOL_FALSE);
        advance();
        return bool_false;
    case TOK_OPEN_PAREN:
        advance();
        Index subexpr = parse_expr();
        expect(TOK_CLOSE_PAREN);
        advance();
        return subexpr;
    case TOK_IDENTIFIER:
        Index ident = new_node(PN_EXPR_IDENT);
        advance();
        return ident;
    case TOK_NUMERIC:
        Index numeric = new_node(PN_EXPR_INT);
        advance();
        return numeric;
    }
    report_token(true, p.cursor, "expected an expression");
    return 0;
}

Index parse_atomic() {
    Index expr = parse_atomic_terminal();
    while (true) {
        switch (current()->kind) {
        case TOK_OPEN_PAREN: {
            Index call = new_node(PN_EXPR_CALL);
            node(call)->lhs = expr;
            advance();
            
            u32 sp = save_dynbuf();

            while (current()->kind != TOK_CLOSE_PAREN) {
                Index arg;
                if (current()->kind == TOK_KEYWORD_OUT) {
                    arg = new_node(PN_OUT_ARGUMENT);
                    advance();
                    Index value = parse_expr();
                    node(arg)->lhs = value;
                } else {
                    arg = parse_expr();
                }

                da_append(&dynbuf, arg);

                if (current()->kind == TOK_COMMA) {
                    advance();
                } else {
                    break;
                }
            }
            expect(TOK_CLOSE_PAREN);
            advance();

            // copy into hard buffer
            Index list = new_extra_with(PNExtraList, dynbuf.len - dynbuf.start);
            as_extra(PNExtraList, list)->len = dynbuf.len - dynbuf.start;
            for_range(i, dynbuf.start, dynbuf.len) {
                as_extra(PNExtraList, list)->items[i - dynbuf.start] = dynbuf.at[i];
            }
            node(call)->rhs = list;
            restore_dynbuf(sp);

            expr = call;
            break;
        }
        case TOK_OPEN_BRACKET:
            Index array_access = new_node(PN_EXPR_INDEX);
            node(array_access)->lhs = expr;
            advance();
            Index indexer = parse_expr();
            expect(TOK_CLOSE_BRACKET);
            advance();
            node(array_access)->rhs = indexer;
            expr = array_access;
            break;
        case TOK_DOT:
            Index selector = new_node(PN_EXPR_SELECTOR);
            node(selector)->lhs = expr;
            advance();
            expect(TOK_IDENTIFIER);
            node(selector)->rhs = p.cursor;
            advance();
            expr = selector;
            break;
        case TOK_CARET:
            Index deref = new_node(PN_EXPR_DEREF);
            node(deref)->lhs = expr;
            expr = deref;
            advance();
            break;
        default:
            return expr;
        }
    }
    return 0;
}

Index parse_unary() {
    switch (current()->kind){
    case TOK_KEYWORD_CAST: {
        Index cast = new_node(PN_EXPR_CAST);
        advance();
        Index expr = parse_expr();
        node(cast)->lhs = expr;
        expect(TOK_KEYWORD_TO);
        advance();
        Index type = parse_type();
        node(cast)->rhs = type;
        return cast;
    }
    case TOK_KEYWORD_CONTAINEROF: {
        Index containerof = new_node(PN_EXPR_CONTAINEROF);
        advance();
        Index expr = parse_expr();
        node(containerof)->lhs = expr;
        expect(TOK_KEYWORD_TO);
        advance();
        Index selector = parse_atomic();
        node(containerof)->rhs = selector;
        return containerof;
    }
    case TOK_AND: u8 simple_kind = PN_EXPR_ADDRESS; goto simple_op;
    case TOK_MINUS: simple_kind = PN_EXPR_NEGATE; goto simple_op;
    case TOK_TILDE: simple_kind = PN_EXPR_BIT_NOT; goto simple_op;
    case TOK_KEYWORD_NOT: simple_kind = PN_EXPR_BOOL_NOT; goto simple_op;
    case TOK_KEYWORD_SIZEOFVALUE: simple_kind = PN_EXPR_SIZEOFVALUE; goto simple_op;
        simple_op:
        Index simple_index = new_node(simple_kind);
        advance();
        Index unary = parse_unary();
        node(simple_index)->lhs = unary;
        return simple_index;
    case TOK_KEYWORD_SIZEOF:
        Index size_of = new_node(PN_EXPR_SIZEOF);
        advance();
        Index type = parse_type();
        node(size_of)->lhs = type;
        return size_of;
    case TOK_KEYWORD_OFFSETOF:
        Index offset_of = new_node(PN_EXPR_OFFSETOF);
        advance();
        Index selector = parse_atomic();
        node(offset_of)->lhs = selector;
        return offset_of;
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
        case TOK_EQ_EQ:
        case TOK_NOT_EQ:
            return 3;
        case TOK_KEYWORD_AND:
            return 2;
        case TOK_KEYWORD_OR:
            return 1;
    }
    return -1;
}

static u8 bin_kind(u8 token_kind) {
    if (TOK_PLUS <= token_kind && token_kind <= TOK_KEYWORD_OR) {
        return token_kind - TOK_PLUS + PN_EXPR_ADD;
    }
    return 0;
}

Index parse_binary(isize precedence) {
    Index lhs = parse_unary();
    

    while (precedence < bin_precedence(current()->kind)) {
        u8 n_prec = bin_precedence(current()->kind);
        Index n = new_node(bin_kind(current()->kind));
        node(n)->lhs = lhs;
        advance();
        Index bin = parse_binary(n_prec);
        node(n)->rhs = bin;
        lhs = n;
    }
    return lhs;
}

ParseTree parse_file(TokenBuf tb) {
    p.tb = tb;
    p.cursor = 0;
    p.tree = (ParseTree){0};
    p.tree.head = 1;

    p.tree.nodes.len = 0;
    p.tree.nodes.cap = 128;
    p.tree.nodes.at = malloc(sizeof(p.tree.nodes.at[0]) * p.tree.nodes.cap);
    p.tree.nodes.kinds = malloc(sizeof(p.tree.nodes.kinds[0]) * p.tree.nodes.cap);

    p.tree.extra.len = 0;
    p.tree.extra.cap = 128;
    p.tree.extra.at = malloc(sizeof(p.tree.extra.at[0]) * p.tree.extra.cap);

    // initialize dynbuf
    da_init(&dynbuf, 128);

    // occupy null node
    if (new_node(0) != 0) {
        CRASH("null node is not at index 0");
    }

    while (current()->kind != TOK_EOF) {
        Index decl = parse_stmt();
    }

    da_destroy(&dynbuf);

    return p.tree;
}