/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"

int main(int argc, char *argv[], char *envp[])
{
    int n = printf(argv[1]);
    printf("\nprinted %d characters\n", n);
    return 0;
}

__attribute__((used))
void _start(int argc, char *argv[], char *envp[], auxv_t *auxv)
{
    init(auxv, false);
    exit(main(argc, argv, envp));
}
