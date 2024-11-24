// #define NO_COLOR

#include "front.h"

#ifdef NO_COLOR
#   define Reset ""
#   define Bold ""
#   define Dim ""
#   define Italic ""
#   define Underline ""

#   define Black ""
#   define Red ""
#   define Green ""
#   define Yellow ""
#   define Blue ""
#   define Magenta ""
#   define Cyan ""
#   define White ""
#   define Default ""

#   define BG_Black ""
#   define BG_Red ""
#   define BG_Green ""
#   define BG_Yellow ""
#   define BG_Blue ""
#   define BG_Magenta ""
#   define BG_Cyan ""
#   define BG_White ""
#   define BG_Default ""
#else
#   define Reset "\x1b[0m"
#   define Bold "\x1b[1m"
#   define Dim "\x1b[2m"
#   define Italic "\x1b[3m"
#   define Underline "\x1b[4m"

#   define Black "\x1b[30m"
#   define Red "\x1b[31m"
#   define Green "\x1b[32m"
#   define Yellow "\x1b[33m"
#   define Blue "\x1b[34m"
#   define Magenta "\x1b[35m"
#   define Cyan "\x1b[36m"
#   define White "\x1b[37m"
#   define Default "\x1b[39m"

#   define BG_Black "\x1b[40m"
#   define BG_Red "\x1b[41m"
#   define BG_Green "\x1b[42m"
#   define BG_Yellow "\x1b[43m"
#   define BG_Blue "\x1b[44m"
#   define BG_Magenta "\x1b[45m"
#   define BG_Cyan "\x1b[46m"
#   define BG_White "\x1b[47m"
#   define BG_Default "\x1b[49m"
#endif

static string find_line(string source, char* location, usize* line, usize* col) {
    
    // out of source range
    if (!(source.raw <= location && location <= (source.raw + source.len))) {
        CRASH("out of source range");
    }

    *line = 1;
    *col = 0;

    char* start_of_current_line = source.raw;

    for_range(i, 0, source.len) {
        if (source.raw[i] == '\n') {
            start_of_current_line = &source.raw[i + 1];
            ++*line;
            *col = 0;
        }

        if (&source.raw[i] == location) {
            usize len = 1;
            while (i + len < source.len && 
                   source.raw[i + len] != '\r' && 
                   source.raw[i + len] != '\n') {
                len++;
            }
            return (string){start_of_current_line, &source.raw[i + len] - start_of_current_line};
        }

        ++*col;
    }

    return NULL_STR;
}

void emit_report(bool error, string source, string path, string highlight, char* message, va_list varargs) {

    char* color = error ? Red : Yellow;

    if (error) {
        printf(""Bold Red"ERROR"Reset);
    } else {
        printf(""Bold Yellow"WARNING"Reset);
    }

    usize line_num = 0;
    usize col_num = 0;

    string line = find_line(source, highlight.raw, &line_num, &col_num);

    printf(" ["str_fmt":%d:%d] ", str_arg(path), line_num, col_num);
    vprintf(message, varargs);

    // trim the highlight if necessary
    if (highlight.raw + highlight.len > line.raw + line.len) {
        highlight.len = line.raw + line.len - highlight.raw;
    }

    printf("\n %zu | ", line_num);
    for_range(i, 0, line.len) {
        if (line.raw + i == highlight.raw) {
            printf(Bold "%s", color);
            // printf(Underline);
        } else if (line.raw + i >= highlight.raw + highlight.len) {
            printf(Reset);
        }

        printf("%c", line.raw[i]);
    }
    printf(Reset);
    printf("\n");

    for(usize l = line_num; l != 0; l /= 10) {
        printf(" ");
    }
    printf("  |"Bold"%s", color);
    for_range(i, 0, line.len + 1) {
        if (line.raw + i <= highlight.raw) {
            printf(" ");
        } else if (line.raw + i <= highlight.raw + highlight.len) {
            printf("~");
        } else {
            break;
        }
    }
    printf(Reset"\n");
    
    if (error) exit(-1);
}