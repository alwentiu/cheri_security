#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>

#include "library.h"
#include "library2.h"


int main(){

	printf("in main, calling test library func\n");
//	test2();

	test();	

	return	0;
}
