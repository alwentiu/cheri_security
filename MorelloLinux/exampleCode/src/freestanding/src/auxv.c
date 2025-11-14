/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "types.h"
#include "auxv.h"

const char *getauxname(unsigned long id)
{
    switch(id) {
        case AT_NULL: return "AT_NULL";
        case AT_IGNORE: return "AT_IGNORE";
        case AT_EXECFD: return "AT_EXECFD";
        case AT_PHDR: return "AT_PHDR";
        case AT_PHENT: return "AT_PHENT";
        case AT_PHNUM: return "AT_PHNUM";
        case AT_PAGESZ: return "AT_PAGESZ";
        case AT_BASE: return "AT_BASE";
        case AT_FLAGS: return "AT_FLAGS";
        case AT_ENTRY: return "AT_ENTRY";
        case AT_NOTELF: return "AT_NOTELF";
        case AT_UID: return "AT_UID";
        case AT_EUID: return "AT_EUID";
        case AT_GID: return "AT_GID";
        case AT_EGID: return "AT_EGID";
        case AT_CLKTCK: return "AT_CLKTCK";
        case AT_PLATFORM: return "AT_PLATFORM";
        case AT_HWCAP: return "AT_HWCAP";
        case AT_FPUCW: return "AT_FPUCW";
        case AT_DCACHEBSIZE: return "AT_DCACHEBSIZE";
        case AT_ICACHEBSIZE: return "AT_ICACHEBSIZE";
        case AT_UCACHEBSIZE: return "AT_UCACHEBSIZE";
        case AT_IGNOREPPC: return "AT_IGNOREPPC";
        case AT_SECURE: return "AT_SECURE";
        case AT_BASE_PLATFORM: return "AT_BASE_PLATFORM";
        case AT_RANDOM: return "AT_RANDOM";
        case AT_HWCAP2: return "AT_HWCAP2";
        case AT_EXECFN: return "AT_EXECFN";
        case AT_SYSINFO: return "AT_SYSINFO";
        case AT_SYSINFO_EHDR: return "AT_SYSINFO_EHDR";
        case AT_L1I_CACHESHAPE: return "AT_L1I_CACHESHAPE";
        case AT_L1D_CACHESHAPE: return "AT_L1D_CACHESHAPE";
        case AT_L2_CACHESHAPE: return "AT_L2_CACHESHAPE";
        case AT_L3_CACHESHAPE: return "AT_L3_CACHESHAPE";
        case AT_L1I_CACHESIZE: return "AT_L1I_CACHESIZE";
        case AT_L1I_CACHEGEOMETRY: return "AT_L1I_CACHEGEOMETRY";
        case AT_L1D_CACHESIZE: return "AT_L1D_CACHESIZE";
        case AT_L1D_CACHEGEOMETRY: return "AT_L1D_CACHEGEOMETRY";
        case AT_L2_CACHESIZE: return "AT_L2_CACHESIZE";
        case AT_L2_CACHEGEOMETRY: return "AT_L2_CACHEGEOMETRY";
        case AT_L3_CACHESIZE: return "AT_L3_CACHESIZE";
        case AT_L3_CACHEGEOMETRY: return "AT_L3_CACHEGEOMETRY";
        case AT_MINSIGSTKSZ: return "AT_MINSIGSTKSZ";
        case AT_CHERI_EXEC_RW_CAP: return "AT_CHERI_EXEC_RW_CAP";
        case AT_CHERI_EXEC_RX_CAP: return "AT_CHERI_EXEC_RX_CAP";
        case AT_CHERI_INTERP_RW_CAP: return "AT_CHERI_INTERP_RW_CAP";
        case AT_CHERI_INTERP_RX_CAP: return "AT_CHERI_INTERP_RX_CAP";
        case AT_CHERI_STACK_CAP: return "AT_CHERI_STACK_CAP";
        case AT_CHERI_SEAL_CAP: return "AT_CHERI_SEAL_CAP";
        case AT_CHERI_CID_CAP: return "AT_CHERI_CID_CAP";
        case AT_ARGC: return "AT_ARGC";
        case AT_ARGV: return "AT_ARGV";
        case AT_ENVC: return "AT_ENVC";
        case AT_ENVP: return "AT_ENVP";
        default: return "<unknown>";
    }
}

unsigned long getauxval(unsigned long id);

size_t getpagesize()
{
    return getauxval(AT_PAGESZ);
}
