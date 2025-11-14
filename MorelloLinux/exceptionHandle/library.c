#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <signal.h>

#include <cheriintrin.h>

#include "library.h"

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


void sigHandler(int sig){
	printf("in signal handler\n");
	printf("pcc: %#p\n", cheri_pcc_get()); 
	printf("ddc: %#p\n", get_csp());
}

void test(){

	printf("pcc: %#p\n", cheri_pcc_get()); 
	printf("ddc: %#p\n", get_csp());
	printf("\n");

	signal(SIGINT, sigHandler);
}

