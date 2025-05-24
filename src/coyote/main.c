#include <stdio.h>

#define ORBIT_IMPLEMENTATION
#include "common/orbit.h"

#include "lex.h"
#include "parse.h"

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

    Parser p = lex_entrypoint(&f);
    for_n(i, 0, p.tokens_len) {
        Token* t = &p.tokens[i];
        if (_TOK_LEX_IGNORE < t->kind) {
            continue;
        }
        printf(str_fmt, str_arg(tok_span(*t)));
        printf(" ");
    }
    printf("\n");

    ty_init();

    Expr* e = parse_expr(&p);
}
