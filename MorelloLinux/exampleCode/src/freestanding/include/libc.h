/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "types.h"

// Kernel ABI constants
#define SYS_EXIT_GROUP 94
#define SYS_MMAP 222
#define SYS_WRITE 64
#define SYS_MPROTECT 226
#define SYS_MUNMAP 215

// Some useful builtins
#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define alloca(n)       __builtin_alloca(n)
#define static_assert   _Static_assert
#define offsetof(type, member) __builtin_offsetof(type, member)
#define alignof(type) _Alignof(type)

// Syscall wrappers
void exit(int code) __attribute__((noreturn));
ssize_t write(int fd, const void *buf, size_t count);
void *mmap(void *addr, size_t len, int prot, int flags);
int mprotect(void *addr, size_t len, int prot);
int munmap(void *addr, size_t len);

// Formatted output
int printf(const char *fmt, ...);
int sprintf(char *dst, const char *fmt, ...);

// String manipulation
size_t strlen(const char *str);
char *strcpy(char *dst, const char *src);
int strcmp(const char *lhs, const char *rhs);
void *memset(void *dst, int c, size_t len);
void *memcpy(void *dst, const void *src, size_t len);

// Init things
int init(const auxv_t *auxv, bool restricted);

// Auxv access
void *getauxptr(unsigned long id);
unsigned long getauxval(unsigned long id);
const char *getauxname(unsigned long id);

// Misc functions
size_t getpagesize();
