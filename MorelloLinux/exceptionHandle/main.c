#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
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


int main(){

	printf("in binary\n");

	printf("pcc: %#p\n", cheri_pcc_get()); 
	//printf("csp: %#p\n", get_csp());

	test();	

	raise(SIGINT);

	return	0;
}
