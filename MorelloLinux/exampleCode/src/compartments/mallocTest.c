/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdbool.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h> // for dlsym
#include <link.h>
#include <errno.h>
#include <signal.h>

#include "cmpt.h"
#include "morello.h"

static void* __capability get_csp(){

	void* __capability ret;

	__asm__(
		"mov %[reg], csp\n"
		: [reg] "=r" (ret)
		:
		:		
	);

	return ret;
}


static void* __capability get_cid_el0(){

    void* __capability ret;

	__asm__(
		"MRS %[reg], CID_EL0\n"
		: [reg] "=r" (ret)
		:
		:		
	);

	return ret;
}

// static void set_cid_el0(void* __capability set){

// 	__asm__(
// 		"MSR CID_EL0, %[reg]\n"
// 		: [reg] "=r" (set)
// 		:
// 		:		
// 	);

// }

static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
    printf("callback():\n");
    printf(" Name: \"%s\" (%d segments)\n", info->dlpi_name,
                  info->dlpi_phnum);
    printf(" - stack (CSP):             %#p\n", cheri_csp_get());
    printf(" - program counter (PCC):   %#p\n", cheri_pcc_get());
    printf(" - base address:            %#p\n", (void*) info->dlpi_addr);

    return 0;
}



void scanCap(void** __capability cap, int depth){
    if(depth > 1){
        return;
    }

    printf("%*sScanning cap: %#p\n", depth * 4, "", cap);
    size_t len = cheri_length_get(cap);

    ptraddr_t start = cheri_base_get(cap);
    void** new_cap = cheri_address_set(cap, start);

    printf("%*s%#p, valid: (%d)\n\n", depth*4, "", new_cap, cheri_tag_get(new_cap));

    if(cheri_tag_get(new_cap) == 0){
        return;
    }

	for(int i = 0; i < len/sizeof(void*); i+=sizeof(void*)){
        access((char*)(new_cap+i), 0);
        // printf("%*s(access returns: %d, %d: %s)\n", depth * 4, "", access_ret, errno, strerror(errno));
        if(errno == 14){
            printf("%*s%#p Segfault, avoiding address\n",depth * 4, "", new_cap+i);
            continue;
        }

        void* __capability new = *((void** __capability)new_cap+i);
        printf("%*s%#p: cap found: %#p\n", depth * 4, "", new_cap+i, new);

        if(cheri_address_get(new) != 0 && cheri_tag_get(new) == 1 && cheri_base_get(new) != start && cheri_length_get(new) >= sizeof(void*)){
            scanCap(new, depth+1);

        }
	}
    printf("\tdone..\n\n");
}

int scanStack(){

	void* __capability csp = get_csp();
	printf("\n\ncomaprt. csp cap: %#p\n", csp);
    printf("starting scan...\n\n");

	size_t len = cheri_length_get(csp);

	for(int i = 0; i < len/sizeof(void*); i+=sizeof(void*)){
        access((char*)(csp-i), 0);
        //printf("(%#p)(access returns: %d: %s)\n", (csp-i), errno, strerror(errno));
        if(errno == 14){
            printf("%#p Segfault, avoiding address\n", csp-i);
            continue;
        }
        
        void* __capability new = *((void** __capability)csp-i);
        if(cheri_tag_get(new)){
            printf("Valid cap found: %#p\n", new);
            //scanCap(new, 1);
        }
	}

	return 0;

}

/**
 * Functions with vulnerability.
 */
static void *get_password(void *buffer)
{

    // doesn't work if statically compiled
    // doesn't have access to the stack either.
    void* dlopen_ret = dlopen(NULL, RTLD_NOW);
    printf(" - dlopen:                  %#p\n", dlopen_ret);

    void* __capability map = *((void** __capability)dlopen_ret);
    printf(" - map:                     %#p\n", map);

    // testing stack walking
    // void* __capability csp = get_csp();
    // printf("csp : %#p\n", csp);
    printf("\n\n**inside get_password**\n\n");

    dl_iterate_phdr(callback, NULL);

    void* __capability cid = get_cid_el0();
    printf("\n\ncompart cid: %#p\n", cid);

    // don't need to set this to malloc scavenge..?
    // printf("setting cid...\n");
    // __asm__(
    //     "mov x0, 0x0\n"
	// 	"MSR CID_EL0, c0\n"
	// 	:
	// 	:		
	// );


    // printf("scanning stack:\n");
    // scanStack();


    // testing malloc scavenging
    printf("\nold array addr: %#p\n\n", buffer);	
    for(int i = 0; i < 15; i++){
        void** array2 = malloc(sizeof(int*) * 4);
        //printf("array in compart: %#p\n", array2);
    	if(array2 == buffer){
		    printf("found copy at iteration %d\n", i);
        	printf("array in compart: %#p\n", array2);
            printf("deref: %#p\n", array2[0]);
            int* myInt = array2[0];
            *myInt = 0x4141;
            break;
	    }   
        free(array2);
    }
    return buffer;
}

static int run_with_cmpt(void* oldArray)
{
    init_cmpt_manager(2000);
    //char authenticated = 0;
    //char buffer[8];
    cmpt_fun_t *get_password_in_cmpt = create_cmpt(get_password, 3 /* pages */, NULL /* use default settings */);
    if (!get_password_in_cmpt) {
        perror("create_cmpt");
        return 1;
    }
    //while(!authenticated) {
    get_password_in_cmpt(oldArray);
    	//if (check_password(get_password_in_cmpt(oldArray), sizeof(buffer))) {
        //    authenticated = 1;
        //}
    //}
    printf("password check passed: have some biscuits\n");
    return 0;
}

static int run_without_cmpt(void* oldArray)
{
    //char authenticated = 0;
    //char buffer[8];
    
    get_password(oldArray);
    /*while(!authenticated) {
        if (check_password(get_password(oldArray), sizeof(buffer))) {
            authenticated = 1;
        }
    }*/
    printf("password check passed: have some biscuits\n");
    return 0;
}

int main(int argc, char *argv[])
{

    void* dlopen_ret = dlopen(NULL, RTLD_LAZY);
    printf(" - dlopen:                  %#p\n", dlopen_ret);
    if(dlopen_ret == 0){
        printf("Error: %s\n", strerror(errno));
    }


    dl_iterate_phdr(callback, NULL);

    void* __capability csp = get_csp();
    printf("maincsp : %#p\n", csp);

    void* __capability cid = get_cid_el0();
    printf("main cid: %#p\n", cid);

    int myInt = 10;
    printf("&myInt : %#p\n", &myInt);

    printf("&myInt: %#p = %d\n", &myInt, myInt);
    void** array = malloc(sizeof(int*) * 4);
    void** array2 = malloc(sizeof(int*) * 4);

    array2[0] = &myInt;

    printf("a1: %#p\n", array);
    printf("a2: %#p\n", array2);

    void* oldArray = array2;
    free(array2);

    array2 = malloc(sizeof(int*) * 4);
    printf("alloc again: %#p\n", array2);
    // malloc scavenging doesn't work here
    printf("reading from re-allocated a2: %#p\n", array2[0]);
    free(array2);

    run_with_cmpt(oldArray);

    printf("myInt: %d\n", myInt);

    if (argc > 1) {
        switch (argv[1][0]) {
        case '1':
            printf("running with compartment...\n");
            return run_with_cmpt(oldArray);
        case '0':
        default:
            printf("running without compartment...\n");
            return run_without_cmpt(oldArray);
        }
    } else {
        fprintf(stderr, "usage: %s <n> where <n> is either 1 or 0\n", argv[0]);
        return 1;
    }
}
