#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h> // for dlsym
#include <link.h>
#include <elf.h>
#include <errno.h>

#include "morello.h"


struct dso {
	unsigned char *base;

	char *name;
	size_t *dynv;
	struct dso *next, *prev;

	Elf64_Phdr *phdr;
	int phnum;
	size_t phentsize;
	Elf64_Sym *syms;
	uint32_t *hashtab;
	uint32_t *ghashtab;
	int16_t *versym;
	char *strings;
	struct dso *syms_next, *lazy_next;
	size_t *lazy, lazy_cnt;
	unsigned char *map;
	size_t map_len;
};


static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
    printf("callback():\n");
    printf(" Name: \"%s\" (%d segments)\n", info->dlpi_name,
                  info->dlpi_phnum);
    printf(" - stack (CSP):             %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf(" - program counter (PCC):   %s\n", cap_to_str(NULL, cheri_pcc_get()));
    printf(" - base address:            %s\n", cap_to_str(NULL, (void *) info->dlpi_addr));

    return 0;
}


int main(int argc, char *argv[])
{

    printf("main():\n");
    printf(" - stack (CSP):             %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf(" - sentry (main):           %s\n", cap_to_str(NULL, &main));
    printf(" - program counter (PCC):   %s\n", cap_to_str(NULL, cheri_pcc_get()));
    printf(" - printf address:          %s\n", cap_to_str(NULL, printf));

    void *glob = dlsym(RTLD_DEFAULT, "_rtld_global");
    printf(" - _rtld_global (dlsym)     %s\n", cap_to_str(NULL, glob));

    void* dlopen_ret = dlopen(NULL, RTLD_NOW);
    printf(" - dlopen:                  %s\n", cap_to_str(NULL, dlopen_ret));

    struct dso *dso_struct = (struct dso* ) dlopen_ret;
    printf(" - dso:                     %s\n", cap_to_str(NULL, dso_struct));
    printf(" - dso name:                %s\n", dso_struct->name);
    printf(" - dso base:                 %#p\n", dso_struct->base);

    ptraddr_t start = cheri_base_get(dlopen_ret);
    void** new_cap = cheri_address_set(dlopen_ret, start);

    void* __capability map = *((void** __capability)new_cap);
    printf(" - dso map:                 %#p\n", map);

    // pointer arithmetic
    printf(" * printf+10 address:       %s\n", cap_to_str(NULL, printf+10));

    dl_iterate_phdr(callback, NULL);

    return 0;
}
