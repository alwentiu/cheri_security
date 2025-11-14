/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cheriintrin.h>
#include "perms.h"

/**
 * Returns length of a capability:
 * length = limit - base
 * Length of a NULL capability is returned as 0.
 */
size_t cheri_length_get_zero(const void * __capability cap);
/**
 * Returns limit of the capability as a pointer.
 * Note that limit for a NULL pointer is returned as NULL.
 */
const void * __capability cheri_get_limit(const void * __capability cap);

/**
 * Returns difference between the address and the limit
 * of a capability: tail = limit - (base + offset) if this
 * capability is in bounds, otherwise, returns NULL.
 */
size_t cheri_get_tail(const void * __capability cap);

/**
 * Returns true if capability is in bounds.
 */
bool cheri_in_bounds(const void * __capability cap);

/**
 * Returns true if a capability is dereferenceable:
 * capability's tag is set, it is not sealed and it
 * is in bounds.
 */
bool cheri_is_deref(const void * __capability cap);

/**
 * Returns true if capability is local.
 */
bool cheri_is_local(const void * __capability cap);

/**
 * Convert capability into string representation.
 * If dst is NULL, a pointer to a static object is returned.
 * Using it may be unsafe.
 */
const char *cap_to_str(char *dst, const void * __capability cap);
const char *cap_perms_to_str(char *dst, const void * __capability cap);
const char *cap_seal_to_str(char *dst, const void * __capability cap);

#if defined(__GNUC__) && !defined(__clang__) && defined(__CHERI__)
// GCC does not provide this builtin.
inline static void * __capability __builtin_cheri_stack_get()
{
    void * __capability ret;
    __asm__ volatile ("mov %0, csp" : "=C"(ret));
    return ret;
}
#endif

#ifndef cheri_csp_get
#define cheri_csp_get() __builtin_cheri_stack_get()
#endif

/**
 * Access CID register.
 */
inline static void * __capability __builtin_cheri_cid_get()
{
    void * __capability ret;
    __asm__ volatile ("mrs %0, CID_EL0" : "=C"(ret));
    return ret;
}

#ifndef cheri_cid_get
#define cheri_cid_get() __builtin_cheri_cid_get()
#endif

/**
 * Create LPB-sealed sentry.
 */
inline static void * __capability morello_lpb_sentry_create(void *cap)
{
    void * __capability ret;
    __asm__ ("seal %0, %1, lpb" : "=C"(ret) : "C"(cap));
    return ret;
}

/**
 * Create LB-sealed sentry.
 */
inline static void * __capability morello_lb_sentry_create(void *cap)
{
    void * __capability ret;
    __asm__ ("seal %0, %1, lb" : "=C"(ret) : "C"(cap));
    return ret;
}

/**
 * Checks if given permissions are present in the capability.
 */
inline static bool cheri_check_perms(const void * __capability cap, size_t perms)
{
    return (cheri_perms_get(cap) | ~perms) == ~0;
}

#ifndef cheri_copy_from_high
#define cheri_copy_from_high(cap) __builtin_cheri_copy_from_high(cap)
#endif
