/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdbool.h>

#include "morello.h"

typedef struct
{
    int x __attribute__((cheri_no_subobject_bounds));
    float y __attribute__((cheri_no_subobject_bounds));
    double z __attribute__((cheri_no_subobject_bounds));
    bool b;
    char *str __attribute__((cheri_no_subobject_bounds));
    char data[] __attribute__((cheri_no_subobject_bounds));
} object_t;

typedef struct
{
    int x;
    float y;
    double z;
    bool b;
    char *str;
    char data[];
} safe_object_t;

int main(int argc, char *argv[])
{
    object_t p = {};
    printf("object:                 %s\n", cap_to_str(NULL, &p));
    printf("address of int x:       %s\n", cap_to_str(NULL, &p.x));
    printf("address of float y:     %s\n", cap_to_str(NULL, &p.y));
    printf("address of double z:    %s\n", cap_to_str(NULL, &p.z));
    printf("address of bool b:      %s\n", cap_to_str(NULL, &p.b));
    printf("address of char *str:   %s\n", cap_to_str(NULL, &p.str));
    printf("address of char data[]: %s\n", cap_to_str(NULL, &p.data));

    safe_object_t q = {};
    printf("safe object:            %s\n", cap_to_str(NULL, &q));
    printf("address of int x:       %s\n", cap_to_str(NULL, &q.x));
    printf("address of float y:     %s\n", cap_to_str(NULL, &q.y));
    printf("address of double z:    %s\n", cap_to_str(NULL, &q.z));
    printf("address of bool b:      %s\n", cap_to_str(NULL, &q.b));
    printf("address of char *str:   %s\n", cap_to_str(NULL, &q.str));
    printf("address of char data[]: %s\n", cap_to_str(NULL, &q.data));

    return 0;
}
