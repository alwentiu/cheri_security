/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

typedef unsigned long size_t;
typedef long ssize_t;
typedef _Bool bool;

#include "morello.h"

#define NULL ((void *)0)

size_t cheri_length_get_zero(const void * __capability cap)
{
    bool is_null = cap == NULL && cheri_tag_get(cap) == 0ul;
    return is_null ? 0ul : cheri_length_get(cap);
}

const void * __capability cheri_get_limit(const void * __capability cap)
{
    return cheri_address_set(cap, cheri_base_get(cap)) + cheri_length_get_zero(cap);
}

size_t cheri_get_tail(const void * __capability cap)
{
    size_t address = cheri_address_get(cap);
    size_t base = cheri_base_get(cap);
    size_t limit = cheri_length_get_zero(cap) + base;
    if (base <= address && address < limit) {
        return limit - address;
    } else {
        return 0ul;
    }
}

bool cheri_in_bounds(const void * __capability cap)
{
    size_t address = cheri_address_get(cap);
    size_t base = cheri_base_get(cap);
    return base <= address && address < (base + cheri_length_get(cap));
}

bool cheri_is_deref(const void * __capability cap)
{
    return cheri_is_valid(cap) && cheri_in_bounds(cap) && !cheri_is_sealed(cap);
}

bool cheri_is_local(const void * __capability cap)
{
    return !cheri_check_perms(cap, PERM_GLOBAL);
}
