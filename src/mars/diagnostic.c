#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/util.h"
#include "common/vec.h"
#include "common/str.h"
#include "common/type.h"
#include "common/ansi.h"

typedef struct Report Report;
typedef struct ReportFile ReportFile;
typedef struct ReportLabel ReportLabel;
typedef struct ReportNote ReportNote;

typedef enum : u8 {
    REPORT_FATAL,
    REPORT_ERROR,
    REPORT_WARN,
    REPORT_INFO,
} ReportKind;

typedef struct Report {

    ReportFile* next;

    ReportNote* notes; // global notes, not attached to a file
} Report;

typedef struct ReportFile {
    string path;
    string source;

    ReportLabel* labels;
    ReportNote* notes;

    ReportFile* next;
} ReportFile;

typedef struct ReportLabel {
    usize start;
    usize len;
    string message;
    bool only_bounds;

    ReportLabel* next;
} ReportLabel;

typedef struct ReportNote {
    string message;

    ReportNote* next;
} ReportNote;

Vec_typedef(string);

static Vec(string) split_lines(string source) {
    usize count = 1; // extra for files that dont end in a newline
    for_n(i, 0, source.len) {
        if (source.raw[i] == '\n') {
            count += 1;
        }
    }

    Vec(string) lines = vec_new(string, count);

    usize cursor = 0;

    while (cursor < source.len) {
        usize len = 0;
        while (cursor + len < source.len) {
            char c = source.raw[cursor + len];
            if (c == '\n') {
                break;
            }
            len++;
        }
        string line = {
            .raw = &source.raw[cursor],
            .len = len,
        };

        

    }

    return lines;
}

int main() {
    string s = strlit("hello\n \n \n");

    Vec(string) lines = split_lines(s);

    for_n(i, 0, lines.len) {
        printf("["str_fmt"]", str_arg(lines.at[i]));
    }
}

/*
warning: forward declaration of PRIVATE function in a separate file
   ╭╴src/fooo.hjk:2:1
   │
 2 │ EXTERN FN bar(IN x: WORD): WORD
   │ ^~~~~~ EXTERN (forward) declaration
  ╶╯
    ╭╴src/bazz.jkl:10:1
    │
  1 │ #INCLUDE "fooo.hjk"
    │ ^~~~~~~~~~~~~~~~~~~ included here
    :
 10 │ PRIVATE FN bar(IN x: WORD): WORD
    │ ^~~~~~~ PRIVATE definition
   ╶╯
    note: forward declarations of PRIVATE functions should reside
    in the same file as the definition
*/