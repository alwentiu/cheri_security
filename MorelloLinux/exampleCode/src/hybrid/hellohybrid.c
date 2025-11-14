/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <alloca.h>

#include "morello.h"

// Capability-aware strcpy:
static char * __capability strcpy_cap(char * __capability dst, const char *src);

// Capability-unaware strcpy:
static char *strcpy(char *dst, const char *src);

// Capability-aware alloca:
// Todo: bounds unrepresentability may cause bounds to overlap other objects.
static inline void * __capability alloca_cap(size_t n)
{
    return cheri_perms_and(cheri_bounds_set((void * __capability)alloca(n), n), PERM_GLOBAL | READ_CAP_PERMS | WRITE_CAP_PERMS);
}

int main(int argc, char *argv[])
{

    printf("hello morello in hybrid mode\n");
    printf("DDC: %s\n", cap_to_str(NULL, cheri_ddc_get()));
    printf("PCC: %s\n", cap_to_str(NULL, cheri_pcc_get()));

    printf("sizeof(ptr) = %zu\n", sizeof(char *));
    printf("sizeof(cap) = %zu\n", sizeof(char * __capability));

    char *ptr = "hybrid";
    char * __capability cap = "hybrid";

    printf("ptr: %016lx\n", (size_t)ptr);
    printf("cap: %s\n", cap_to_str(NULL, cap));

    // Make some capability:
    char * __capability dst_cap = alloca_cap(20);

    // Use capability-aware strcpy:
    char * __capability res_cap = strcpy_cap(dst_cap, "hello hybrid");
    printf("dst: %s\n", cap_to_str(NULL, dst_cap));
    printf("res: %s\n", cap_to_str(NULL, res_cap));

    // Use non-capability-aware strcpy:
    char * res_addr = strcpy((char *)cheri_address_get(dst_cap), "hello hybrid");
    printf("res: %016lx\n", (size_t)res_addr);

    return 0;
}

static char * __capability strcpy_cap(char * __capability dst, const char *src)
{
    for(; cheri_is_deref(dst) && *src; src++, dst++) {
        *dst = *src;
    }
    if (cheri_is_deref(dst)) {
        *dst = '\0';
    }
    return dst;
}

static char *strcpy(char *dst, const char *src)
{
    for(; *src; src++, dst++) {
        *dst = *src;
    }
    *dst = '\0';
    return dst;
}
