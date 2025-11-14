/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"
#include "rcmpt.h"

__attribute__((noinline,used))
int sum(int x, int y)
{
    long cid = get_compartment_id();
    printf("[%ld] csp: %s\n", cid, cap_to_str(NULL, cheri_csp_get()));
    printf("[%ld] pcc: %s\n", cid, cap_to_str(NULL, cheri_pcc_get()));
    return x + y;
}

__attribute__((noinline,used))
int sum_with_nested_cmpt(switch_t *cmpt, int x, int y)
{
    long cid = get_compartment_id();
    printf("[%ld] csp: %s\n", cid, cap_to_str(NULL, cheri_csp_get()));
    printf("[%ld] pcc: %s\n", cid, cap_to_str(NULL, cheri_pcc_get()));
    return cmpt(x, y);
}

/**
 * Restricted main function running in restricted mode.
 * It runs in a so-called root compartment. All functions
 * invoked directly will remain in this compartment, but
 * any functions invoked via a compartment instance will
 * run on separate stack.
 */
int main(int argc, char *argv[], char *envp[])
{
    printf("[%ld] hello restricted mode\n", get_compartment_id());

    // Direct calls just work because branching via a label inherits
    // current PCC permissions:
    printf("2 + 3 = %d\n", sum(2, 3));

    // However, using an indirect call to `sum` would result in a tag
    // fault when returning from it if we hadn't set up the relocation
    // for the `sum` correctly (without the EXECUTIVE perm) (see the
    // `init` invocation in the `_init_compartments` in `src/cman.c`):
    int(*fnp)(int, int) = &sum;
    printf("2 + 3 = %d\n", fnp(2, 3));

    // Call the `sum` function in a compartment:
    switch_t *cmp0 = create_compartment(sum, 2 /* pages */);
    printf("2 + 3 = %d\n", cmp0(2, 3));

    // Create second compartment:
    switch_t *cmp1 = create_compartment(sum, 3 /* pages */);
    printf("2 + 3 = %d\n", cmp1(2, 3));

    // Nested compartments:
    switch_t *cmp2 = create_compartment(sum_with_nested_cmpt, 1 /* pages */);
    printf("3 + 8 = %d\n", cmp2((intptr_t)cmp0, 3, 8));

    return 0;
}
