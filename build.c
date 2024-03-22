/**
 * Copyright (C) 2024 Amrit Bhogal
 *
 * This file is part of CBuild.
 *
 * CBuild is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CBuild.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "cbuild.h"
#include "pkg-config.h"

target(test, EXECUTABLE) {
    add_glob("test-src/*.c");
    add_include_directories("test-src");
    add_flags("-Wall", "-Wextra", "-Werror");

    struct PkgConfigResult result = pkg_config("raylib");
    if (not result.success) {
        fprintf(stderr, "Failed to find raylib");
        exit(1);
    }
    add_package(result);
};
