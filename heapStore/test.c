#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "library.h"
#include "library2.h"


int main(){

	printf("in main, calling test library func\n");
	test2();

	int testInt = 10;

	test();	

	void** testAlloc = malloc(0x10);
	testAlloc[0] = &testInt;
//	printf("allocated and saved at: %#p\n\n", testAlloc);

	for(int i = 0; i < 100000; i++){
		void* alloc1 = malloc(0x10);
		free(alloc1);
	}

	testAlloc = malloc(0x10);
	testAlloc[0] = &testInt;
	//printf("allocated at: %#p\n\n", testAlloc);

	testStore(testAlloc);	

	return	0;
}
