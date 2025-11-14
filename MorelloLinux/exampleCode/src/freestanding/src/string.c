/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

/**
 * A "safe" strlen that would not do an out-of-bounds access.
 * String length is counted from `src` till either a null-
 * terminator or the limit of the capability.
 *
 * A NULL or invalid pointer is treated like an empty string.
 */
size_t strlen(const char *str)
{
    if (!str || !cheri_is_deref(str)) return 0ul;
    size_t k, max = cheri_get_tail(str);
    for (k = 0; k < max && *str; str++, k++);
    return k;
}

/**
 * A "safe" implementation of strcpy. The source
 * string is copied into destination until the end
 * of the source string or either destination or the
 * source string reaches its limit, so the limit is
 * treated as a terminator if it occurs before the
 * null character.
 */
char *strcpy(char *dst, const char *src)
{
    const char *dst_lim = cheri_get_limit(dst);
    const char *src_lim = cheri_get_limit(src);
    for(; dst < dst_lim && src < src_lim && *src; dst++, src++) {
        *dst = *src;
    }
    if (dst < dst_lim) {
        *dst = '\0';
    }
    return dst;
}

/**
 * A "safe" string compare function. The characters are
 * compared until the end of any of the strings or either
 * one of them reaches its limit which is treated as a
 * terminator if it occurs before the null character.
 */
int strcmp(const char *lhs, const char *rhs)
{
    const char *lhs_lim = (char *)cheri_get_limit(lhs);
    const char *rhs_lim = (char *)cheri_get_limit(rhs);
    for(; lhs < lhs_lim && rhs < rhs_lim && *lhs == *rhs && *lhs; lhs++, rhs++);
    if (lhs < lhs_lim && rhs < rhs_lim) {
        return *(unsigned char *)lhs - *(unsigned char *)rhs;
    } else if (lhs < lhs_lim) {
        return *(unsigned char *)lhs;
    } else if (rhs < rhs_lim) {
        return -(*(unsigned char *)rhs);
    } else {
        return 0;
    }
}

/**
 * A simple bounds-checking memset.
 */
void *memset(void *dst, int c, size_t len)
{
    size_t max = cheri_get_tail(dst);
    if (len > max) {
        len = max;
    }
    if (len == 0ul) {
        return dst;
    }
    char *tail_end = dst + len;
    char *tail;
    if (len < sizeof(void *)) {
        tail = dst;
        goto tail;
    } else {
        tail = cheri_align_down(tail_end, sizeof(void *));
    }
    char *head = dst;
    char *head_end = cheri_align_up(head, sizeof(void *));
    for(char *x = head; x < head_end; x++) {
        *x = c;
    }
    uint64_t b = c;
    uint64_t t = (b << 56) | (b << 48) | (b << 40) | (b << 32) | (b << 24) | (b << 16) | (b << 8) | b;
    for(uint64_t *x = (uint64_t *)head_end; x < (uint64_t *)tail; x+=2) {
        x[0] = t;
        x[1] = t;
    }
tail:
    for(char *x = tail; x < tail_end; x++) {
        *x = c;
    }
    return dst;
}

/**
 * A simple bounds-checking and capability-aware memcpy.
 * All 16-byte aligned blocks will be copied as capabilities
 * preserving memory tags unless the source and the destination
 * are misaligned.
 */
void *memcpy(void *dst, const void *src, size_t len)
{
    void *res = dst;
    size_t max_dst = cheri_get_tail(dst);
    size_t max_src = cheri_get_tail(src);
    if (len > max_dst) {
        len = max_dst;
    }
    if (len > max_src) {
        len = max_src;
    }
    if (len == 0ul) {
        return res;
    }
    bool misaligned = (((uint64_t)dst) & 0xful) != (((uint64_t)src) & 0xful);
    if (misaligned || len < sizeof(void *)) { // copy bytes
        char *d = (char *)dst;
        char *s = (char *)src;
        for(size_t k = 0; k < len; d++, s++, k++) {
            *d = *s;
        }
    } else { // capability-aware copying
        char *head = (char *)src;
        char *head_end = cheri_align_up(head, sizeof(void *));
        char *tail_end = head + len;
        char *tail = cheri_align_down(tail_end, sizeof(void *));
        char *d = dst;
        for(char *s = head; s < head_end; d++, s++) {
            *d = *s;
        }
        for(void **s = (void **)head_end; s < (void **)tail; d += sizeof(void *), s++) {
            *((void **)d) = *s;
        }
        for(char *s = tail; s < tail_end; d++, s++) {
            *d = *s;
        }
    }
    return res;
}
