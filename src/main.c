#define ORBIT_IMPLEMENTATION

#include "jacc.h"
#include "front.h"

JaccContext ctx;

SourceFile get_source(string path) {
    fs_file f;
    fs_get(path, &f);
    
    string text = string_alloc(f.size);
    printf("f.size %d\n", f.size);
    fs_open(&f, "rb");
    fs_read_entire(&f, text.raw);
    fs_close(&f);
    fs_drop(&f);

    return (SourceFile){
        .path = path,
        .text = text,
    };
}

void add_source_to_ctx(SourceFile src) {
    if (ctx.sources.at == NULL) da_init(&ctx.sources, 8);
    da_append(&ctx.sources, src);
}

int main(int argc, char** argv) {
    printf("hello world!\n");

    SourceFile src = get_source(str("./test.jkl"));
    printf("src %d\n", src.text.len);
    add_source_to_ctx(src);
    TokenBuf tb = lex_tokenize(&src);

    for_range(i, 0, tb.len) {
        printf("% 2d    %.*s\n", tb.at[i].len, tb.at[i].len, tb.at[i].raw);
    }
}