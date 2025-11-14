/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "morello.h"

long global_variable = 42l;

int main(int argc, char *argv[])
{
    long double local_variable = 3.14;

    printf("hello morello\n");
    printf("here are some capabilities pointing to...\n");

    // objects and functions
    printf(" - local var (long double): %s\n", cap_to_str(NULL, &local_variable));
    printf(" - stack (CSP):             %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf(" - global var (long):       %s\n", cap_to_str(NULL, &global_variable));
    printf(" - sentry (main):           %s\n", cap_to_str(NULL, &main));
    printf(" - program counter (PCC):   %s\n", cap_to_str(NULL, cheri_pcc_get()));

    // memory mappings
    void *private_rw_mapping = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf(" - private rw mapping:      %s\n", cap_to_str(NULL, private_rw_mapping));
    void *shared_rw_mapping = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    printf(" - shared rw mapping:       %s\n", cap_to_str(NULL, shared_rw_mapping));
    int fd = open("/proc/self/exe", O_RDONLY);
    void *private_ro_file_mapping = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
    printf(" - private ro file mapping: %s\n", cap_to_str(NULL, private_ro_file_mapping));
    close(fd);

    // misc capabilities
    printf(" - AT_CHERI_SEAL_CAP:       %s\n", cap_to_str(NULL, getauxptr(AT_CHERI_SEAL_CAP)));
    printf(" - AT_CHERI_CID_CAP:        %s\n", cap_to_str(NULL, getauxptr(AT_CHERI_CID_CAP)));
    printf(" - AT_ARGV:                 %s\n", cap_to_str(NULL, getauxptr(AT_ARGV)));
    printf(" - AT_ENVP:                 %s\n", cap_to_str(NULL, getauxptr(AT_ENVP)));
    printf(" - AT_CHERI_STACK_CAP:      %s\n", cap_to_str(NULL, getauxptr(AT_CHERI_STACK_CAP)));

    return 0;
}
