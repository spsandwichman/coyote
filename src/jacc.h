#pragma once

#include "orbit.h"

typedef struct SourceFile {
    string path;
    string text;
} SourceFile;
da_typedef(SourceFile);

typedef struct JaccContext {
    da(SourceFile) sources;
} JaccContext;

extern JaccContext ctx;