#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cheriintrin.h>

#include "library.h"

int myGlobal = 0;

int main(){

	int myInt = 0;
	void** myHeap = malloc(sizeof(void*));

	printf("in main, calling test library func\n");
	printf("my pcc: %#p\n", cheri_pcc_get());
	printf("my int: %#p\n", &myInt);
	printf("my global: %#p\n", &myGlobal);
	printf("my heap: %#p\n", myHeap);

	test();	

	return	0;
}
