/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "cmpt.h"
#include "morello.h"

// This function will run inside compartment
static void *fun(void *arg)
{
    printf("inside...\n");
    printf("csp: %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf("cid: %s\n", cap_to_str(NULL, cheri_cid_get()));
    printf("pcc: %s\n", cap_to_str(NULL, cheri_pcc_get()));
    *((int *)arg) = 0;
    return arg;
}

int main(int argc, char const *argv[])
{
    init_cmpt_manager(1000);

    // Create compartment for the "fun" function
    cmpt_flags_t flags = {
        .pcc_system_reg = false,
        .stack_store_local = false,
        .stack_mutable_load = true
    };
    cmpt_fun_t *fun_in_cmpt = create_cmpt(fun, 4 /* pages */, &flags);
    if (!fun_in_cmpt) {
        perror("create_cmpt");
        return 1;
    }

    int arg = 9;

    // Stack before:
    printf("before...\n");
    printf("csp: %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf("cid: %s\n", cap_to_str(NULL, cheri_cid_get()));
    printf("pcc: %s\n", cap_to_str(NULL, cheri_pcc_get()));

    // Invoke function in the compartment
    int *res = fun_in_cmpt(&arg);

    // Stack after:
    printf("after...\n");
    printf("csp: %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf("cid: %s\n", cap_to_str(NULL, cheri_cid_get()));
    printf("pcc: %s\n", cap_to_str(NULL, cheri_pcc_get()));
    return *res;
}
