/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#define intptr_t __INTPTR_TYPE__
#define uintptr_t __UINTPTR_TYPE__

typedef long ssize_t;
typedef unsigned long size_t;
typedef int int32_t;
typedef long int64_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef _Bool bool;
typedef __builtin_va_list va_list;
typedef ssize_t ptrdiff_t;

#define NULL ((void *)(0))
#define true 1
#define false 0

typedef struct {
    uint64_t type;
    union {
        uint64_t val;
        void *ptr;
    };
} auxv_t;
