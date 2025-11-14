#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <signal.h>

#include <cheriintrin.h>

#include "library.h"

void* testLib(){
	return dlopen("libc.so.7", RTLD_NOW);
}



struct link_map
{
/* These first few members are part of the protocol with the debugger.
	This is the same format used in SVR4.  */

	void* l_addr;		/* Difference between the address in the ELF
					file and the addresses in memory.  */
	char *l_name;		/* Absolute file name object was found in.  */
	void* *l_ld;		/* Dynamic section of the shared object.  */
	struct link_map *l_next, *l_prev; /* Chain of loaded objects.  */
};


int test(){
	
	printf("\n\nInside malicious library\n");
	printf("My pcc: %#p\n", cheri_pcc_get());

	void* lib2 = dlopen(NULL, RTLD_NOW);

	void** testCap = (void**) lib2;

	struct link_map *test = (struct link_map*) lib2;

	printf("l_addr: %#p\nl_name: %s\nl_ld: %#p\n\n", test->l_addr, test->l_name, test->l_ld);

	struct link_map *prev = test->l_prev;
	struct link_map *next = test->l_next;

	while(prev != NULL){
		printf("l_addr: %#p\nl_name: %s\nl_ld: %#p\n", prev->l_addr, prev->l_name, prev->l_ld);

		prev = prev->l_prev;
	}

	while(next != NULL){
		printf("l_addr: %#p\nl_name: %s\nl_ld: %#p\n\n", next->l_addr, next->l_name, next->l_ld);

		next = next->l_next;
	}

//	print_obj(lib);

	return 0;

}
