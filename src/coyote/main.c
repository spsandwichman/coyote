#include <stdio.h>

#include "common/orbit.h"

#include "common/util.h"
#include "lex.h"
#include "parse.h"

thread_local const char* filepath = nullptr;
thread_local FlagSet flags = {};

static void parse_args(int argc, char** argv) {
    for_n(i, 0, argc) {
        char* arg = argv[i];
        if (strcmp(arg, "--strict") == 0) {
            flags.strict = true;
        } else if (strcmp(arg, "--error-on-warn") == 0) {
            flags.error_on_warn = true;
        } else if (arg[0] == '-') {
            printf("unknown flag '%s'\n", arg);
            exit(1);
        } else {
            filepath = arg;
        }
    }
}

int main(int argc, char** argv) {

    parse_args(argc, argv);

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
    p.flags = flags;
    
    // p.flags.strict = true;
    // p.flags.error_on_warn = true;

    // for_n(i, 0, p.tokens_len) {
    //     Token* t = &p.tokens[i];
    //     if (_TOK_LEX_IGNORE < t->kind) {
    //         if (t->kind == TOK_NEWLINE) {
    //             // printf("\n");
    //             continue;
    //         }
    //         printf("%s ", token_kind[t->kind]);
    //         continue;
    //     }
    //     if (t->kind == TOK_STRING) {
    //         printf("\"");
    //     }
    //     printf(str_fmt, str_arg(tok_span(*t)));
    //     if (t->kind == TOK_STRING) {
    //         printf("\"");
    //     }
    //     printf(" ");
    // }
    // printf("\n");

    CompilationUnit cu = parse_unit(&p);
}
