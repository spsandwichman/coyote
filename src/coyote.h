#pragma once

#include "orbit.h"

typedef struct SourceFile {
    string path;
    string text;
} SourceFile;
da_typedef(SourceFile);

typedef struct CoyoteContext {
    da(SourceFile) sources;
} CoyoteContext;

extern CoyoteContext ctx;