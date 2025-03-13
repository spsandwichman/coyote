#ifndef ORBIT_NEWFS_H
#define ORBIT_NEWFS_H

#define _XOPEN_SOURCE 700

#include "type.h"
#include "str.h"
#include "vec.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(OS_WINDOWS)

#elif defined(OS_LINUX)
    #include <limits.h>
#else
    #error "buh"
#endif

typedef struct FsPath {
    u16 len;
    char raw[PATH_MAX];
} FsPath;

typedef struct FsFile {
    isize handle;
    usize id;
    usize size;
    // units unspecified, but greater means more recent
    usize youth;
    FsPath path;
} FsFile;

#define fs_from_path(pathptr) (string){.len = (pathptr)->len, .raw = (pathptr)->raw}
bool fs_real_path(const char* path, FsPath* out);
FsFile* fs_open(const char* path, bool create, bool overwrite);
usize fs_read(FsFile* f, void* buf, usize len);
string fs_read_entire(FsFile* f);
void fs_close(FsFile* f);

bool fs_exists(const char* path);
bool fs_is_directory(const char* path);
bool fs_is_file(const char* path);

Vec_typedef(string);
// returns contents. if contents == NULL, return a newly allocated vec.
Vec(string) fs_dir_contents(const char* path, Vec(string)* contents);

#endif // ORBIT_NEWFS_H