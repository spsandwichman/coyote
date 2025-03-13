#include "fs.h"

#ifdef OS_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool fs_real_path(const char* path, FsPath* out) {
    if (GetFullPathNameA(path, PATH_MAX, out->raw, NULL) == 0) {
        return false;
    }
    out->len = strlen(out->raw);

    // fuck backslash all my homies hate backslash
    for_n(i, 0, out->len) {
        if (out->raw[i] == '\\') {
            out->raw[i] = '/';
        }
    }

    return true;
}

FsFile* fs_open(const char* path, bool create, bool overwrite) {
    FsFile* f = malloc(sizeof(FsFile));
    if (create) {
        f->handle = (void*) CreateFileA(
            path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
            overwrite ? CREATE_ALWAYS : CREATE_NEW,
            0, 0
        );
    } else {
        f->handle = (void*) CreateFileA(
            path, GENERIC_READ, FILE_SHARE_READ, NULL, 
            overwrite ? OPEN_ALWAYS : OPEN_EXISTING,
            0, 0
        );
    }
    if (f->handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    BY_HANDLE_FILE_INFORMATION info;
    GetFileInformationByHandle((HANDLE)f->handle, &info);

    ULARGE_INTEGER windows_why_are_u_say_zis;

    windows_why_are_u_say_zis.HighPart = info.nFileIndexHigh;
    windows_why_are_u_say_zis.LowPart  = info.nFileIndexLow;
    f->id = (usize)windows_why_are_u_say_zis.QuadPart;

    windows_why_are_u_say_zis.HighPart = info.ftLastWriteTime.dwHighDateTime;
    windows_why_are_u_say_zis.LowPart  = info.ftLastWriteTime.dwLowDateTime;
    f->youth = (usize)windows_why_are_u_say_zis.QuadPart;

    windows_why_are_u_say_zis.HighPart = info.nFileSizeHigh;
    windows_why_are_u_say_zis.LowPart  = info.nFileSizeLow;
    f->size = (usize)windows_why_are_u_say_zis.QuadPart;

    fs_real_path(path, &f->path);

    return f;
}

// returns how much was /actually/ read.
usize fs_read(FsFile* f, void* buf, usize len) {
    DWORD num_read = 0;
    ReadFile((HANDLE)f->handle, buf, len, &num_read, NULL);
    return num_read;
}

string fs_read_entire(FsFile* f) {
    string buf = string_alloc(f->size);
    DWORD num_read;
    ReadFile((HANDLE)f->handle, buf.raw, buf.len, &num_read, NULL);
    return buf;
}

void fs_close(FsFile* f) {
    CloseHandle((HANDLE)f->handle);
}

Vec(string) fs_dir_contents(const char* path, Vec(string)* _contents) {

    Vec(string) contents = {0};

    usize path_len = strlen(path);
    char* search_path = malloc(path_len + 3);
    memcpy(search_path, path, path_len);
    search_path[path_len] = '/';
    search_path[path_len + 1] = '*';
    search_path[path_len + 2] = '\0';


    if (_contents == NULL) {
        contents = vec_new(string, 16);
    } else {
        contents = *contents;
    }

    WIN32_FIND_DATAA data;
    HANDLE search_handle = FindFirstFileA(search_path, &data);
    do {
        if (strcmp(data.cFileName, ".")) continue;
        if (strcmp(data.cFileName, "..")) continue;
        printf(":: %s\n", data.cFileName);
        vec_append(&contents, string_clone(str(data.cFileName)));

    } while (FindNextFileA(search_handle, &data));

    FindClose(search_handle);
    free(search_path);
    return contents;
}

#endif