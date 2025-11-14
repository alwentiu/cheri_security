/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#include "morello.h"

/**
 * Wrappable function type.
 */
typedef void *(cmpt_fun_t)(void* arg);

/**
 * Compartment handle type (opaque).
 */
typedef struct {
    void *data[2];
} cmpt_t;

static const cmpt_t *create_cmpt(cmpt_fun_t *target, unsigned stack_pages);

// See src/lpb.S
extern void *cmpt_call(const cmpt_t *cmpt, void *arg);
extern void *cmpt_switch(void *arg);

static void *fun(void *buffer)
{
    printf("inside...\n");
    printf("csp: %s\n", cap_to_str(NULL, cheri_csp_get()));
    int *data = buffer;
    int x = data[0];
    int y = data[1];
    int z = x + y;
    data[2] = z;
    return data;
}

int main(int argc, char *argv[])
{
    const cmpt_t *cmpt = create_cmpt(fun, 4 /* pages */);
    int buffer[3] = { 2, 3, 0 };

    printf("before...\n");
    printf("csp: %s\n", cap_to_str(NULL, cheri_csp_get()));

    int *res = cmpt_call(cmpt, buffer);

    printf("after...\n");
    printf("csp: %s\n", cap_to_str(NULL, cheri_csp_get()));

    printf("result: %d + %d = %d\n", res[0], res[1], res[2]);
    return 0;
}

#define STACK_PERMS (PERM_GLOBAL | READ_CAP_PERMS | WRITE_CAP_PERMS)

/**
 * Compartment descriptor
 */
typedef struct {
    void *stack;
    void *target;
} cmpt_desc_t;

static const cmpt_t *create_cmpt(cmpt_fun_t *target, unsigned stack_pages)
{
    size_t pgsz = getpagesize();
    size_t sz = pgsz * stack_pages;

    void *stack = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // Note: setting bounds is going to be redundant here
    // once kernel returns bounded capability.
    stack = cheri_bounds_set(stack, sz);
    stack = cheri_offset_set(stack, sz);
    stack = cheri_perms_and(stack, STACK_PERMS);

    void *data = mmap(NULL, pgsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cmpt_desc_t *desc = (cmpt_desc_t *)cheri_bounds_set_exact(data, sizeof(cmpt_desc_t));
    cmpt_t *cmpt = (cmpt_t *)cheri_bounds_set_exact(data + sizeof(cmpt_desc_t), sizeof(cmpt_t));
    if (!cheri_is_sealed(target)) {
        target = cheri_sentry_create(target);
    }
    // Compartment descriptor:
    desc->stack = stack;
    desc->target = target;
    // Capability pair:
    cmpt->data[0] = cheri_perms_and(desc, PERM_GLOBAL | READ_CAP_PERMS); // data capability
    cmpt->data[1] = cmpt_switch; // code capability
    // Return read-only LPB-sealed pointer to cap pair:
    return morello_lpb_sentry_create(cheri_perms_and(cmpt, PERM_GLOBAL | READ_CAP_PERMS));
}
