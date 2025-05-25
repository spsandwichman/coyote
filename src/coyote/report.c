#include <stdint.h>
#include <stdio.h>

#include "lex.h"
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

static void print_snippet(string line, string snippet, const char* color, usize pad) {
    
    fprintf(stderr, "| ");

    for_n(i, 0, line.len) {
        if (line.raw + i == snippet.raw) {
            fprintf(stderr, Bold "%s", color);
        }
        if (line.raw + i == snippet.raw + snippet.len) {
            fprintf(stderr, Reset);
        }
        fprintf(stderr, "%c", line.raw[i]);
    }
    fprintf(stderr, Reset);
    fprintf(stderr, "\n");
    for_n(i, 0, pad) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "| ");
    for_n(i, 0, line.len) {
        if (line.raw + i < snippet.raw) {
            fprintf(stderr, " ");
        }
        if (line.raw + i == snippet.raw) {
            fprintf(stderr, Bold "%s^", color);
        }
        if (line.raw + i == snippet.raw + snippet.len) {
            fprintf(stderr, "\n");
            break;
        }
        if (line.raw + i > snippet.raw) {
            fprintf(stderr, "~");
        }
    }
    fprintf(stderr, Reset);
}

bool is_whitespace(char c) {
    switch (c) {
    case ' ':
    case '\r':
    case '\n':
    case '\t':
    case '\v':
        return true;
    }
    return false;
}

string snippet_line(string src, string snippet) {
    string line = snippet;
    // expand line backwards
    while (true) {
        if (line.raw == src.raw) break;
        if (*line.raw == '\n') {
            line.raw++;
            break;
        }
        line.raw--;
    }
    // trim leading whitespace
    while (is_whitespace(line.raw[0])) {
        line.raw++;
    }
    // expand line forwards
    while (true) {
        if (line.raw + line.len == src.raw + src.len) break;
        if (line.raw[line.len] == '\n') {
            // line.len--;
            break;
        }
        line.len++;
    }
    return line;
}

void report_line(ReportLine* report) {

    const char* color = White;

    switch (report->kind) {
    case REPORT_ERROR: fprintf(stderr, Bold Red"ERROR"Reset); color = Red; break;
    case REPORT_WARNING: fprintf(stderr, Bold Yellow"WARN"Reset); color = Yellow; break;
    case REPORT_NOTE: fprintf(stderr, Bold Cyan"NOTE"Reset); color = Cyan; break;
    }

    u32 line_num = line_number(report->src, report->snippet);
    u32 col_num  = col_number(report->src, report->snippet);

    fprintf(stderr, " ["Bold str_fmt Reset":%u:%u] ", str_arg(report->path), line_num, col_num);
    fprintf(stderr, str_fmt, str_arg(report->msg));
    fprintf(stderr, "\n");

    if (report->reconstructed_line.raw != nullptr) {
        fprintf(stderr, "%4u ", line_num);
        // fprintf(stderr, "     ");
        string line = snippet_line(report->reconstructed_line, report->reconstructed_snippet);
        print_snippet(line, report->reconstructed_snippet, color, 5);
        fprintf(stderr, "expanded from: "Reset"\n");
    }
    
    fprintf(stderr, "%4u ", line_num);
    string line = snippet_line(report->src, report->snippet);
    print_snippet(line, report->snippet, color, 5);
    
    if (report->kind == REPORT_ERROR) {
        exit(3);
    }
}
