#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "library.h"



int main(){

	printf("in main, calling test library func\n");

	int testInt = 10;

	int** testArray = malloc(0x10);
	int** testArray2 = malloc(0x10);
	free(testArray2);
	int** testArray3 = malloc(0x10);

	printf("%#p\n", testArray);

	testArray2[0] = &testInt;
	testArray3[0] = &testInt;


	free(testArray3);

	printf("%#p\n", testArray2);
	printf("%#p\n", testArray3);
	printf("\n\n");
	test(testArray3);	


	return	0;
}
