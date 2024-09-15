#include "front.h"

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