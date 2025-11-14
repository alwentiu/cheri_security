/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

void exit(int code)
{
    register intptr_t c8 __asm__("c8") = SYS_EXIT_GROUP;
    register intptr_t c0 __asm__("c0") = code;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "0"(c0));
    __builtin_unreachable();
}

ssize_t write(int fd, const void *buf, size_t count)
{
    size_t tail = cheri_get_tail(buf);
    if (count > tail) {
        count = tail;
    }
    if (count == 0ul) {
        return 0ul;
    }
    register intptr_t c8 __asm__("c8") = SYS_WRITE;
    register intptr_t c0 __asm__("c0") = fd;
    register intptr_t c1 __asm__("c1") = (intptr_t)buf;
    register intptr_t c2 __asm__("c2") = count;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "0"(c0), "C"(c1), "C"(c2));
    return c0;
}

void *mmap(void *addr, size_t len, int prot, int flags)
{
    register intptr_t c8 __asm__("c8") = SYS_MMAP;
    register intptr_t c0 __asm__("c0") = (intptr_t)addr;
    register intptr_t c1 __asm__("c1") = len;
    register intptr_t c2 __asm__("c2") = prot;
    register intptr_t c3 __asm__("c3") = flags;
    register intptr_t c4 __asm__("c4") = -1;
    register intptr_t c5 __asm__("c5") = 0;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "C"(c0), "C"(c1), "C"(c2), "C"(c3), "C"(c4), "C"(c5));
    return (void *)cheri_bounds_set(c0, len);
}

int mprotect(void *addr, size_t len, int prot)
{
    register intptr_t c8 __asm__("c8") = SYS_MPROTECT;
    register intptr_t c0 __asm__("c0") = (intptr_t)addr;
    register intptr_t c1 __asm__("c1") = len;
    register intptr_t c2 __asm__("c2") = prot;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "C"(c0), "C"(c1), "C"(c2));
    return (int)c0;
}

int munmap(void *addr, size_t len)
{
    register intptr_t c8 __asm__("c8") = SYS_MUNMAP;
    register intptr_t c0 __asm__("c0") = (intptr_t)addr;
    register intptr_t c1 __asm__("c1") = len;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "C"(c0), "C"(c1));
    return (int)c0;
}
