#include "lex.h"
#include <stdint.h>
#include <stdio.h>
#include <wingdi.h>

#include "common/ansi.h"

static u32 line_number(string src, string snippet) {
    u32 line = 1;
    char* current = src.raw;
    while (current != snippet.raw) {
        if (*current == '\n') line++;
        current++;
    }

    return line;
}

static u32 col_number(string src, string snippet) {
    u32 col = 1;
    char* current = src.raw;
    while (current != snippet.raw) {
        if (*current == '\n') col = 0;
        col++;
        current++;
    }

    return col;
}

void report_line(ReportLine* report) {

    const char* color = White;

    switch (report->kind) {
    case REPORT_ERROR: printf(Bold Red"ERROR"Reset); color = Red; break;
    case REPORT_WARNING: printf(Bold Yellow"WARN"Reset); color = Yellow; break;
    case REPORT_NOTE: printf(Bold Cyan"NOTE"Reset); color = Cyan; break;
    }

    u32 line_num = line_number(report->src, report->snippet);
    u32 col_num  = col_number(report->src, report->snippet);

    printf(" ["Bold str_fmt Reset":%u:%u] ", str_arg(report->path), line_num, col_num);
    printf(str_fmt, str_arg(report->msg));
    printf("\n");

    string line = report->snippet;
    // expand line backwards
    while (true) {
        if (line.raw == report->src.raw) break;
        if (*line.raw == '\n') {
            line.raw++;
            break;
        }
        line.raw--;
    }
    // expand line forwards
    while (true) {
        if (line.raw + line.len == report->src.raw + report->src.len) break;
        if (line.raw[line.len] == '\n') {
            // line.len--;
            break;
        }
        line.len++;
    }

    printf("| ");

    for_n(i, 0, line.len) {
        if (line.raw + i == report->snippet.raw) {
            printf(Bold "%s", color);
        }
        if (line.raw + i == report->snippet.raw + report->snippet.len) {
            printf(Reset);
        }
        printf("%c", line.raw[i]);
    }
    printf("\n| ");
    for_n(i, 0, line.len) {
        if (line.raw + i < report->snippet.raw) {
            printf(" ");
        }
        if (line.raw + i == report->snippet.raw) {
            printf(Bold "%s^", color);
        }
        if (line.raw + i == report->snippet.raw + report->snippet.len) {
            printf(Reset"\n");
            break;
        }
        if (line.raw + i > report->snippet.raw) {
            printf("~");
        }
    }

    if (report->kind == REPORT_ERROR) {
        exit(1);
    }
}