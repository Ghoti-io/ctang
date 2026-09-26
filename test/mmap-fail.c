/*
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Copyright (C) 2026 Corey Pennycuff
 *
 * This file is part of Ghoti.io Tang.
 *
 * Ghoti.io Tang is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Ghoti.io Tang is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Ghoti.io Tang.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * An LD_PRELOAD shim that makes every executable mapping fail.
 *
 * The JIT asks mmap() for one PROT_EXEC page per compiled program, and the
 * arm that handles being refused is not reachable from any input: the test
 * suite compiles thousands of programs and every one of them is granted its
 * page. Preloading this makes the refusal the only outcome, so that the
 * suite runs the failure arm for every program it compiles.
 *
 * Only PROT_EXEC mappings are refused. malloc() and the C++ runtime map memory
 * without it and are handed the real mmap(), so the process runs
 * normally in every respect except that no program can be JIT compiled.
 *
 * Linux only, and a test artifact: it is never installed.
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

typedef void * (*Mmap_Function)(void *, size_t, int, int, int, off_t);

void * mmap(void * address, size_t length, int protection, int flags, int fd, off_t offset);

void * mmap(void * address, size_t length, int protection, int flags, int fd, off_t offset) {
  static Mmap_Function real_mmap = 0;

  if (protection & PROT_EXEC) {
    errno = ENOMEM;
    return MAP_FAILED;
  }

  if (!real_mmap) {
    // dlsym() hands back an object pointer, which C does not promise can hold
    // a function pointer. POSIX requires this to work, and copying the bytes
    // is how it is spelled without provoking -pedantic-errors.
    void * symbol = dlsym(RTLD_NEXT, "mmap");
    memcpy(&real_mmap, &symbol, sizeof(real_mmap));
  }

  return real_mmap(address, length, protection, flags, fd, offset);
}
