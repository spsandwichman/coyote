#include "jacc.h"
#include "front.h"

#define MACRO_MAX_DEPTH 64

// entrypoint into the preprocessor.
void preproc_dispatch(Lexer* l) {
    lex_advance(l);
    lex_skip_whitespace(l);
}