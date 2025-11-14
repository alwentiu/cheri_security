/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "types.h"

/**
 * Generic function type to accommodate functions with different
 * arguments. A function of this type is returned from the
 * `create_compartment` function below.
 */
typedef intptr_t(switch_t)(intptr_t arg0, ...);

/**
 * Creates an instance of compartment with private stack of the
 * given size `stack_pages`. The `target` argument is the pointer
 * to the target function that will be called in the compartment
 * when the compartment instance is invoked.
 *
 * The returned pointer is a sentry of type `switch_t` which you
 * can use instead of the original target function.
 *
 * Note: the target function mustn't be variadic (this is not
 * supported) and may only have up to 8 arguments.
 */
switch_t *create_compartment(void *target, unsigned stack_pages);

/**
 * Returns current compartment ID.
 * Root compartment has ID = 0 and all manually created compartments
 * have sequential positive IDs.
 * If called from executive mode, this function will return -1.
 */
long get_compartment_id();

/**
 * Checks if we are in restricted mode.
 */
bool is_in_restricted();
