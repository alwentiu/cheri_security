/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/auxv.h>

#include <dlfcn.h> // for dlsym
#include <errno.h>

#include "morello.h"
/**
 * Compartment options.
 */
typedef struct {
    bool pcc_system_reg;        // enables PERM_SYS_REG in compartment
    bool stack_store_local;     // enables STORE_LOCAL perm in stack
    bool stack_mutable_load;    // enables MUTABLE_LOAD perm in stack
} cmpt_flags_t;

/**
 * Wrappable function type.
 */
typedef void *(cmpt_fun_t)(void* arg);

/**
 * Initialise compartment manager.
 */
void init_cmpt_manager(size_t seed);

/**
 * Create compartment entry around a function pointer.
 * Optional flags may be used to modify properties of
 * the compartment. If NULL is used, then the default
 * properties will be used: the PCC perms are defined
 * by the provided target and the perms of the stack
 * are not reduced.
 *
 * Return value: on success, this function returns a
 * callable object (sentry) that can be used in stead
 * of the original target function. On failure NULL is
 * returned and errno is set to indicate the reason.
 */
cmpt_fun_t *create_cmpt(cmpt_fun_t *target, unsigned stack_pages, const cmpt_flags_t *flags);

/**
 * Removes permissions from sentry and returns sentry
 * with fewer permissions. The sentry must be either
 * unsealed or be within the bounds and permissions
 * of the current PCC.
 */
void *reseal_and_remove_perms(void *sentry, size_t perms);

