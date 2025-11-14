/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

typedef unsigned long size_t;
typedef long ssize_t;
typedef _Bool bool;

#include "morello.h"

int sprintf(char *dst, const char *fmt, ...);
char *strcpy(char *dst, const char *src);

#define NULL ((void *)0)
#define TEST(x, f) (((x) & (f)) == (f))

static const char permnames[] = {
    'G',
    'r', 'R', 'M',
    'w', 'W', 'L',
    'x', 'E', 'S',
    's', 'u',
    'I',
    'C',
    'V',
    '1', '2', '3',
    0
};

static size_t permbits[] = {
    PERM_GLOBAL,
    PERM_LOAD, PERM_LOAD_CAP, PERM_MUTABLE_LOAD,
    PERM_STORE, PERM_STORE_CAP, PERM_STORE_LOCAL_CAP,
    PERM_EXECUTE, PERM_EXECUTIVE, PERM_SYS_REG,
    PERM_SEAL, PERM_UNSEAL,
    PERM_CAP_INVOKE,
    PERM_CMPT_ID,
    PERM_VMEM,
    PERM_USER_1, PERM_USER_2, PERM_USER_3,
    0ul
};

static char buf[128];

const char *cap_perms_to_str(char *dst, const void * __capability cap)
{
    if (dst == NULL) {
        dst = buf;
    }
    const char *res = dst;
    size_t perms = cheri_perms_get(cap);
    const size_t *p = permbits;
    const char *n = permnames;
    for(; *p && *n; n++, p++, dst++) {
        *dst = TEST(perms, *p) ? *n : '-';
    }
    *dst = '\0';
    return res;
}

const char *cap_seal_to_str(char *dst, const void * __capability cap)
{
    if (dst == NULL) {
        dst = buf;
    }
    const char *res = dst;
    unsigned int otype = cheri_type_get(cap) & 0x7fffu;
    switch (otype) {
        case 0:
            strcpy(dst, "none");
            break;
        case 1:
            strcpy(dst, "rb");
            break;
        case 2:
            strcpy(dst, "lpb");
            break;
        case 3:
            strcpy(dst, "lb");
            break;
        default:
            sprintf(dst, "%04x", otype);
    }
    return res;
}

/**
 * Prints detailed information about capability to string.
 */
const char *cap_to_str(char *dst, const void * __capability cap)
{
    if (dst == NULL) {
        dst = buf;
    }
    size_t tag = cheri_tag_get(cap);
    size_t addr = cheri_address_get(cap);
    size_t base = cheri_base_get(cap);
    size_t len = cheri_length_get(cap);
    size_t lim = base + len;
    ssize_t offset = (ssize_t)cheri_offset_get(cap);

    char perm[19];
    char seal[5];
    sprintf(dst, "%016lx %c [%016lx:%016lx) %s %-4s %ld of %lu",
        addr, tag ? '1' : '0', base, lim, cap_perms_to_str(perm, cap),
        cap_seal_to_str(seal, cap), offset, len);
    return dst;
}
