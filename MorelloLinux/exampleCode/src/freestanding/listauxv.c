/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

__attribute__((used))
void _start(int argc, char *argv[], char *envp[], auxv_t *auxv)
{
    init(auxv, false);
    for (const auxv_t *entry = auxv; entry->type; entry++) {
        const char *name = getauxname(entry->type);
        if (cheri_is_deref(entry->ptr)) {
            printf(" %-22s %-2lu %+#p\n", name, entry->type, entry->ptr);
        } else {
            printf(" %-22s %-2lu %016lx\n", name, entry->type, entry->val);
        }
    }
    exit(0);
}
