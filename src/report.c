#include "lex.h"

typedef enum {
    REPORT_ERROR,
    REPORT_WARNING,
    REPORT_INFO,
} ReportKind;

typedef struct {
    u8 kind;
    string src; 
    string path; 
    string snippet; 
    string msg;
} ReportLine;

void report_line(ReportLine* line) {

}