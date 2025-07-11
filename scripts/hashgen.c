#include <ctype.h>

#include "../src/common/orbit.h"

#include "../src/common/fs.h"
#include "../src/common/fs_win.c"
#include "../src/common/fs_linux.c"

#include "../src/common/str.c"
#include "../src/common/vec.c"

usize hashfunc(const char* key, usize key_len, usize offset, usize mult, usize table_size) {
    usize hash = offset;

    for (usize i = 0; i < key_len; ++i) {
        hash ^= key[i];
        hash *= mult;
    }
    return hash % table_size;
}

#define func_text \
    "usize hashfunc(const char* key, usize key_len) {\n" \
    "    usize hash = %zu;\n" \
    "\n" \
    "    for (usize i = 0; i < key_len; ++i) {\n" \
    "        hash ^= key[i];\n" \
    "        hash *= %zu;\n" \
    "    }\n" \
    "    return hash %% %zu;\n" \
    "}\n" \

Vec(string) keywords = {};

int main(int argc, char** argv) {

    keywords = vec_new(string, 256);
    // intake keywords file

    FsFile* file = fs_open(argv[1], false, false);
    string text = fs_read_entire(file);
    text = string_concat(text, constr("\n"));

    usize i = 0;
    while (i < text.len) {
        if (isspace(text.raw[i])) {
            ++i;
            continue;
        }
        // scan a keyword
        usize start = i;
        while (!isspace(text.raw[i])) {
            ++i;
        }
        string kw = {
            .raw = &text.raw[start],
            .len = i - start
        };
        vec_append(&keywords, kw);
    }

    // for_n(i, 0, keywords.len) {
    //     printf("["str_fmt"]\n", str_arg(keywords.at[i]));
    // }

    for_n(table_size, keywords.len, 100000000) {
        bool* occupied = malloc(sizeof(bool) * table_size);
        memset(occupied, 0, sizeof(bool) * table_size);

        constexpr usize bits = 13;

        for_n(mult, 1, 1ll << bits) {
            for_n(offset, 1, 1ll << bits) {
                // try this table configuration.
                for_n(i, 0, keywords.len) {
                    string kw = keywords.at[i];
                    usize index = hashfunc(kw.raw, kw.len, offset, mult, table_size);
                    if (occupied[index]) {
                        goto next_config;
                    }
                    occupied[index] = true;
                }
                // config works!!!
                printf("for keywords:");
                for_n(i, 0, keywords.len) {
                    string kw = keywords.at[i];
                    printf("  "str_fmt" -> %llu\n", str_arg(kw), hashfunc(kw.raw, kw.len, offset, mult, table_size));
                }
                printf("CONFIG:\n\n" func_text"\n", offset, mult, table_size);
                return 0;
                    
                next_config:
                memset(occupied, 0, sizeof(bool) * table_size);
            }
        }
        printf("tried table size %llu\n", table_size);
    }

    fs_close(file);
}
