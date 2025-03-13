#define ORBIT_IMPLEMENTATION
#include <stdio.h>
#include "orbit.h"

#include "lex.h"

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("no file provided\n");
        return 1;
    }

    char* filepath = argv[1];
    // fs_file file = {0};
    // if (!fs_get(str(filepath), &file)) return 1;
    // if (!fs_open(&file, "rb")) return 1;

    // string src = string_alloc(file.size + 1);
    // fs_read_entire(&file, src.raw);
    // src.raw[src.len - 1] = ' ';

    FsFile* file = fs_open(filepath, false, false);
    if (file == NULL) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    printf("file "str_fmt"\n", str_arg(file->path));
    printf("size %zu\n", file->size);
    printf("age  %zu\n", file->youth);

    SrcFile f = {
        .src = fs_read_entire(file),
        .path = fs_from_path(&file->path),
    };

    Vec(Token) tokens = lex_entrypoint(&f);

    printf("%zu tokens\n", tokens.len);

    fs_dir_contents(".", NULL);

    // for_vec(Token* t, &tokens) {
    //     if (_TOK_PREPROC_BEGIN < t->kind && t->kind < _TOK_PREPROC_END) {
    //         printf("%s ", token_kind[t->kind]);
    //         continue;
    //     }
    //     printf(str_fmt" ", str_arg(tok_span(*t)));
    // }
}