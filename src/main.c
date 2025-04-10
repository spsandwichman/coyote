#define ORBIT_IMPLEMENTATION
#include <stdio.h>
#include "orbit.h"

#include "lex.h"

#include "iron/iron.h"

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("no file provided\n");
        return 1;
    }

    char* filepath = argv[1];

    FsFile* file = fs_open(filepath, false, false);
    if (file == NULL) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    SrcFile f = {
        .src = fs_read_entire(file),
        .path = fs_from_path(&file->path),
    };

    Vec(Token) tokens = lex_entrypoint(&f);

    // printf("%zu chars -> %zu tokens (%zuB used, %zuB capacity)\n", file->size, tokens.len, sizeof(Token)*tokens.len, sizeof(Token)*tokens.cap);

    for_vec(Token* t, &tokens) {
        if (_TOK_PREPROC_BEGIN < t->kind && t->kind < _TOK_PREPROC_END) {
            printf("%s ", token_kind[t->kind]);
            continue;
        }
        if (t->kind == TOK_STRING) {
            printf("\"");
        }
        printf(str_fmt, str_arg(tok_span(*t)));
        if (t->kind == TOK_STRING) {
            printf("\"");
        }
        printf(" ");
    }

    fe_init_signal_handler();
}