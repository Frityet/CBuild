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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <iso646.h>

#define $argc(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

struct StringList {
    size_t count;
    char **values;
};

static inline void StringList_append(struct StringList *list, const char *value)
{
    if (list->count == 0) {
        list->values = malloc(sizeof(const char *));
    } else {
        list->values = realloc(list->values, (list->count + 1) * sizeof(const char *));
    }

    list->values[list->count++] = strdup(value);
}

static bool StringList_iterate(struct StringList *list, char **outp)
{
    if (list->count == 0) {
        return false;
    }

    *outp = list->values[--list->count];
    return true;
}

static void StringList_print(struct StringList list)
{
    puts("[");
    char *value;
    while (StringList_iterate(&list, &value)) {
        printf("\t\"%s\",\n", value);
    }
    puts("]");
}

static pid_t execute_async(const char *command, size_t argc, const char *argv[])
{
    {
        printf("$ %s", command);
        char buf[1024];
        for (size_t i = 0; i < argc; i++) {
            printf(" %s", argv[i]);
        }
        puts("");
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        char **args = calloc(argc+2, sizeof(char *));
        args[0] = strdup(command);
        for (size_t i = 1; i < argc; i++) {
            args[i] = strdup(argv[i]);
        }

        execvp(command, args);
        perror("execvp");
        exit(1);
    }

    return pid;
}

static const char *execute(const char *command, size_t argc, const char *argv[])
{
    int inout[2];
    if (pipe(inout) == -1) {
        perror("pipe");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return NULL;
    }

    if (pid == 0) {
        close(inout[0]);
        dup2(inout[1], STDOUT_FILENO);
        close(inout[1]);

        char **args = calloc(argc+2, sizeof(char *));
        args[0] = strdup(command);
        for (size_t i = 1; i < argc; i++) {
            args[i] = strdup(argv[i]);
        }

        execvp(command, args);
        perror("execvp");
        exit(1);
    }

    close(inout[1]);
    FILE *out = fdopen(inout[0], "r");
    char *buffer = malloc(1024);
    size_t size = 0;
    while (fgets(buffer + size, 1024 - size, out)) {
        size += strlen(buffer + size);
    }

    fclose(out);
    return buffer;
}

static const char *c_compiler = "cc";
struct BuildInfo;
typedef void Target_f(struct BuildInfo *info);
typedef bool OnFile_f(struct BuildInfo *info, const char *file);

enum ProductType {
    EXECUTABLE,
    STATIC_LIBRARY,
    SHARED_LIBRARY
};

typedef struct BuildInfo {
    enum ProductType type;
    const char *output_name;
    struct StringList files, include_directories, libraries, library_directories, flags;
    Target_f *before_compile, *before_link;
    Target_f *after_compile, *after_link;
    OnFile_f *on_compile;
    struct StringList object_files;
} BuildInfo_t;

#define target(name, type) static Target_f define_build_##name; int main(int argc, const char *argv[]) { return cbuild(#name, type, argc, argv, define_build_##name); }\
static void define_build_##name(BuildInfo_t *$build_info$)

static inline void add_file(BuildInfo_t *info, const char *file)
{
    StringList_append(&info->files, file);
}

static inline void add_files(BuildInfo_t *info, size_t count, const char *files[])
{
    for (size_t i = 0; i < count; i++) {
        add_file(info, files[i]);
    }
}

static inline void add_glob(BuildInfo_t *info, const char *pattern)
{
    glob_t globbuf;
    glob(pattern, 0, NULL, &globbuf);
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        add_file(info, globbuf.gl_pathv[i]);
    }
    globfree(&globbuf);
}

static inline void add_include_directory(BuildInfo_t *info, const char *directory)
{
    StringList_append(&info->include_directories, directory);
}
static inline void add_include_directories(BuildInfo_t *info, size_t count, const char *directories[])
{
    for (size_t i = 0; i < count; i++) {
        add_include_directory(info, directories[i]);
    }
}

static inline void add_library(BuildInfo_t *info, const char *library)
{
    StringList_append(&info->libraries, library);
}

static inline void add_libraries(BuildInfo_t *info, size_t count, const char *libraries[])
{
    for (size_t i = 0; i < count; i++) {
        add_library(info, libraries[i]);
    }
}
static inline void add_library_directory(BuildInfo_t *info, const char *directory)
{
    StringList_append(&info->library_directories, directory);
}
static inline void add_library_directories(BuildInfo_t *info, size_t count, const char *directories[])
{
    for (size_t i = 0; i < count; i++) {
        add_library_directory(info, directories[i]);
    }
}
static inline void add_flag(BuildInfo_t *info, const char *flag)
{
    StringList_append(&info->flags, flag);
}
static inline void add_flags(BuildInfo_t *info, size_t count, const char *flags[])
{
    for (size_t i = 0; i < count; i++) {
        add_flag(info, flags[i]);
    }
}

#define add_file(...) add_file($build_info$, __VA_ARGS__)
#define add_files(...) add_files($build_info$, $argc(__VA_ARGS__), (const char *[]){__VA_ARGS__})
#define add_glob(...) add_glob($build_info$, __VA_ARGS__)
#define add_include_directory(...) add_include_directory($build_info$, __VA_ARGS__)
#define add_include_directories(...) add_include_directories($build_info$, $argc(__VA_ARGS__), (const char *[]){__VA_ARGS__})
#define add_library(...) add_library($build_info$, __VA_ARGS__)
#define add_libraries(...) add_libraries($build_info$, $argc(__VA_ARGS__), (const char *[]){__VA_ARGS__})
#define add_library_directory(...) add_library_directory($build_info$, __VA_ARGS__)
#define add_library_directories(...) add_library_directories($build_info$, $argc(__VA_ARGS__), (const char *[]){__VA_ARGS__})
#define add_flag(...) add_flag($build_info$, __VA_ARGS__)
#define add_flags(...) add_flags($build_info$, $argc(__VA_ARGS__), (const char *[]){__VA_ARGS__})

static const char *const PRODUCT_TYPES[] = {
    [EXECUTABLE] = "executable",
    [SHARED_LIBRARY] = "shared library",
    [STATIC_LIBRARY] = "static library",
};

static int compile(BuildInfo_t *info, pid_t running_compiles[static 1024])
{
    size_t pid_count = 0;
    char *file;

    size_t object_count = 0;
    char *obj_files[1024] = {0};
    while (StringList_iterate(&info->files, &file)) {
        if (info->on_compile && !info->on_compile(info, file)) {
            return 1;
        }

        const char *argv[1024] = {0};
        size_t argc = 0;
        argv[argc++] = c_compiler;
        argv[argc++] = "-c";
        argv[argc++] = file;
        for (size_t i = 0; i < info->include_directories.count; i++) {
            char *ptr;
            asprintf(&ptr, "-I%s", info->include_directories.values[i]);
            argv[argc++] = ptr;
        }

        for (size_t i = 0; i < info->flags.count; i++) {
            argv[argc++] = info->flags.values[i];
        }

        char *output_file;
        asprintf(&output_file, "%s.o", file);
        argv[argc++] = "-o";
        argv[argc++] = output_file;
        obj_files[object_count++] = output_file;

        pid_t pid = execute_async(c_compiler, argc, argv);
        running_compiles[pid_count++] = pid;
    }

    for (size_t i = 0; i < 1024; i++) {
        if (running_compiles[i] == 0) {
            break;
        }

        int status;
        waitpid(running_compiles[i], &status, 0);
        if (status != 0) {
            return status;
        }

        StringList_append(&info->object_files, obj_files[i]);
    }

    return 0;
}

static int link_objects(BuildInfo_t *info)
{
    const char *argv[1024] = {0};
    size_t argc = 0;
    switch (info->type) {
    case EXECUTABLE:
        argv[argc++] = c_compiler;
        break;
    case SHARED_LIBRARY:
        argv[argc++] = c_compiler;
        argv[argc++] = "-shared";
        break;
    case STATIC_LIBRARY:
        argv[argc++] = "ar";
        argv[argc++] = "rcs";
        break;
    }

    for (size_t i = 0; i < info->object_files.count; i++) {
        argv[argc++] = info->object_files.values[i];
    }

    argv[argc++] = "-o";
    char *output_file;
    asprintf(&output_file, "%s%s%s", info->output_name, info->type == STATIC_LIBRARY ? ".a" : "", info->type == SHARED_LIBRARY ? ".so" : "");
    argv[argc++] = output_file;



    for (size_t i = 0; i < info->library_directories.count; i++) {
        char *ptr;
        asprintf(&ptr, "-L%s", info->library_directories.values[i]);
        argv[argc++] = ptr;
    }

    for (size_t i = 0; i < info->libraries.count; i++) {
        char *ptr;
        asprintf(&ptr, "-l%s", info->libraries.values[i]);
        argv[argc++] = ptr;
    }

    for (size_t i = 0; i < info->flags.count; i++) {
        argv[argc++] = info->flags.values[i];
    }

    pid_t pid = execute_async(c_compiler, argc, argv);
    int status;
    waitpid(pid, &status, 0);
    return status;
}

static int cbuild(const char *output_name, enum ProductType type, int argc, const char *argv[], void (*configure)(BuildInfo_t *))
{
    printf("Configuring %s %s\n", PRODUCT_TYPES[type], output_name);
    BuildInfo_t info = {
        .type = type,
        .output_name = output_name,
    };
    configure(&info);

    printf("Files: ");
    StringList_print(info.files);
    printf("Include Directories: ");
    StringList_print(info.include_directories);
    printf("Libraries: ");
    StringList_print(info.libraries);
    printf("Library Directories: ");
    StringList_print(info.library_directories);
    printf("Flags: ");
    StringList_print(info.flags);

    #define $run(x, ...) if (x) x(__VA_ARGS__);
    $run(info.before_compile, &info);
    pid_t running_compiles[1024] = {0};
    int status = compile(&info, running_compiles);
    if (status != 0) {
        return status;
    }
    $run(info.after_compile, &info);

    $run(info.before_link, &info);
    status = link_objects(&info);
    if (status != 0) {
        return status;
    }
    $run(info.after_link, &info);

#undef $run
    printf("Built %s %s\n", PRODUCT_TYPES[type], output_name);

    return 0;
}
