// Copyright (C) 2024 Amrit Bhogal
//
// This file is part of CBuild.
//
// CBuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CBuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CBuild.  If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include "cbuild.h"
#include <stdbool.h>

struct PkgConfigResult {
    bool success;
    const char *package;
    struct StringList cflags, ldflags;
    struct StringList include_directories, library_directories, libraries;
};

static struct PkgConfigResult pkg_config(const char *package)
{
    size_t argc = 0;
    const char *argv[1024];
    argv[argc++] = "--cflags";
    argv[argc++] = "--libs";
    argv[argc++] = package;

    struct PkgConfigResult result = {
        .package = package,
    };

    const char *output = execute("pkg-config", argc, argv);
    if (output == NULL) {
        result.success = false;
        return result;
    }

    //split by space
    for (const char *p = output; *p; p++) {
        if (*p == ' ') {
            continue;
        }

        const char *start = p;
        while (*p && *p != ' ') {
            p++;
        }

        size_t len = p - start;
        char *value = malloc(len + 1);
        memcpy(value, start, len);
        value[len] = '\0';

        if (value[len - 1] == '\n') {
            value[len - 1] = '\0';
        }

        if (strncmp(value, "-I", 2) == 0) {
            StringList_append(&result.include_directories, value + 2);
        } else if (strncmp(value, "-L", 2) == 0) {
            StringList_append(&result.library_directories, value + 2);
        } else if (strncmp(value, "-l", 2) == 0) {
            StringList_append(&result.libraries, value + 2);
        } else {
            StringList_append(&result.cflags, value);
        }
    }

    result.success = true;
    return result;
}

static void add_package(BuildInfo_t *info, struct PkgConfigResult res)
{
    if (!res.success) {
        fprintf(stderr, "Failed to get package information\n");
        return;
    }

    (add_flags)(info, res.cflags.count, (const char **)res.cflags.values);
    (add_include_directories)(info, res.include_directories.count, (const char **)res.include_directories.values);
    (add_library_directories)(info, res.library_directories.count, (const char **)res.library_directories.values);
    (add_libraries)(info, res.libraries.count, (const char **)res.libraries.values);
}

#define add_package(...) add_package($build_info$, __VA_ARGS__)
