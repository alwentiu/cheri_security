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
    cmpt_fun_t *another = (cmpt_fun_t *)arg;
    unsigned cid = cheri_address_get(cheri_cid_get());
    const void *csp = cheri_csp_get();
    printf("%u: calling compartment: %s\n", cid, cap_to_str(NULL, another));
    printf("%u: inside outer,  csp = %s\n", cid, cap_to_str(NULL, csp));
    return another((void *)3);
}

// This function will run inside compartment
static void *nested(void *arg)
{
    unsigned cid = cheri_address_get(cheri_cid_get());
    const void *csp = cheri_csp_get();
    printf("%u: inside nested, csp = %s\n", cid, cap_to_str(NULL, csp));
    return arg;
}

int main(int argc, char const *argv[])
{
    init_cmpt_manager(3000);
    cmpt_fun_t *fun_in_cmpt = create_cmpt(fun, 4 /* pages */, NULL);
    cmpt_fun_t *nested_cmpt = create_cmpt(nested, 4 /* pages */, NULL);
    int r = (int)cheri_address_get(fun_in_cmpt(nested_cmpt));
    return r; // should return 3
}
