#include <stdio.h>

#define ORBIT_IMPLEMENTATION
#include "common/orbit.h"

#include "lex.h"
#include "parse.h"
#include "iron/iron.h"

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("no file provided\n");
        return 1;
    }

    char* filepath = argv[1];

    FsFile* file = fs_open(filepath, false, false);
    if (file == nullptr) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    SrcFile f = {
        .src = fs_read_entire(file),
        .path = fs_from_path(&file->path),
    };

    Context ctx = lex_entrypoint(&f);
    for_n(i, 0, ctx.tokens_len) {
        Token* t = &ctx.tokens[i];
        if (t->kind == TOK_NEWLINE) {
            // printf("\n");
            continue;
        }
        if (_TOK_LEX_IGNORE < t->kind && t->kind < _TOK_PREPROC_TRANSPARENT_END) {
            // printf("%s ", token_kind[t->kind]);
            continue;
        }
        printf(str_fmt, str_arg(tok_span(*t)));
        // token_error(&ctx, i, i + 2, "expression is not an l-value");
        // break;
        printf(" ");
    }
}
