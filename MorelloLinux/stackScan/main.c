#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "library.h"


int main(){


	void* csp;
	asm(
	"mov %[reg], csp\n"
	: [reg] "=r" (csp)
	:
	:
	);

	printf("in main, csp: %#p\ncalling test library func\n", csp);


	int testInt = 10;

	printf("Test int: %d at %#p\n", testInt, &testInt);
	
	test();	

	printf("Test int: %d at %#p\n", testInt, &testInt);

	return 0;
}
