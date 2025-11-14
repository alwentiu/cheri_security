/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "cmpt.h"
#include "morello.h"

static char check_password(const char *buffer, size_t sz)
{
    return strncmp(buffer, "secret", sz) == 0;
}

/**
 * Functions with vulnerability.
 */
static void *get_password(void *buffer)
{
    printf("password: "); fflush(stdout);
    char *p = cheri_address_set(cheri_csp_get(), cheri_address_get(buffer)); // derive cap from stack
    if (!cheri_is_deref(p)) {
        p = buffer;
    }
    scanf("%s", p); // may overflow
    return buffer;
}

static int run_with_cmpt()
{
    init_cmpt_manager(2000);
    char authenticated = 0;
    char buffer[8];
    cmpt_fun_t *get_password_in_cmpt = create_cmpt(get_password, 3 /* pages */, NULL /* use default settings */);
    if (!get_password_in_cmpt) {
        perror("create_cmpt");
        return 1;
    }
    while(!authenticated) {
        if (check_password(get_password_in_cmpt(buffer), sizeof(buffer))) {
            authenticated = 1;
        }
    }
    printf("password check passed: have some biscuits\n");
    return 0;
}

static int run_without_cmpt()
{
    char authenticated = 0;
    char buffer[8];
    while(!authenticated) {
        if (check_password(get_password(buffer), sizeof(buffer))) {
            authenticated = 1;
        }
    }
    printf("password check passed: have some biscuits\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        switch (argv[1][0]) {
        case '1':
            printf("running with compartment...\n");
            return run_with_cmpt();
        case '0':
        default:
            printf("running without compartment...\n");
            return run_without_cmpt();
        }
    } else {
        fprintf(stderr, "usage: %s <n> where <n> is either 1 or 0\n", argv[0]);
        return 1;
    }
}
