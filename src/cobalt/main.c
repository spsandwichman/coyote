#include <stdio.h>
#include <synchapi.h>

#include "common/fs.h"

#include "lex.h"

int main(int argc, char** argv) {

    const char* filepath = argv[1];

    FsFile* file = fs_open(filepath, false, false);
    if (file == nullptr) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    string text;
    text.raw = malloc(file->size + 4); // 4 bytes of 0 as a safety net for the lexer
    text.len = file->size;
    memset(&text.raw[text.len], 0, 4);
    fs_read(file, text.raw, text.len);
    fs_close(file);

    Arena arena;
    arena_init(&arena);
    Lexer l;
    l.arena = &arena;
    l.cursor = 0;
    l.src = text;

    Token t = lex_next_raw(&l);
    do {
        printf(""str_fmt, t.len, t.raw);
        t = lex_next_raw(&l);
        Sleep(100);
    } while (t.kind != TOK_EOF);
}
