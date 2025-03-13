#include "newfs.h"

#ifdef OS_WINDOWS

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
usize fs_read(FsFile* f, u8* buf, usize len) {
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

    WIN32_FIND_DATAA data;
    HANDLE h;
    if (!FindFirstFileA(search_path, &data)) return contents; // .
    FindNextFileA(search_path, &data); // ..

    if (_contents == NULL) {
        // contents = vec_new(string, 16);
    } else {
        // contents = contents.at;
    }

    while (FindNextFileA(search_path, &data)) {
        // printf(":: %s\n", data.cFileName);
    }

    

    

    free(search_path);
    return contents;
}

#endif