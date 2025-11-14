#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "library.h"

int main(){

	printf("in main, calling test library func\n");

	int testInt = 10;
	printf("&testing: %#p\n", &testInt);

	test();	

	void** testAlloc = malloc(0x100);
	testAlloc[0] = &testInt;
	printf("in main, allocated at: %#p\n\n", testAlloc);

	testStore(testAlloc);	

	printf("testInt: %d\n", testInt);

	return	0;
}
