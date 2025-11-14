#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <signal.h>

#include <cheriintrin.h>

#include "library.h"



void*** heapList;
int size = 1000000;

int test(){

	heapList = malloc(sizeof(void*) * size);

	for(int i = 0; i < size; i++){
		void*** new = malloc(0x100);
		heapList[i] = new; 
		free(new);
		//printf("%#p\n", new);
	}

	printf("%#p\n", heapList[0]);

}


void testStore(void* alloc){

	printf("%#p\n", heapList[0]);

	for(int i = 0; i < size; i++){
		//printf("%#p\n", heapList[i]);

		if(alloc == heapList[i]){
			printf("found %#p: %#p\n", heapList[i], *heapList[i]);
			int* intPtr = (int*)(*heapList[i]);
			printf("int: %d\n", *intPtr);
			*intPtr = 0x41;
			return;
		}
	}

}

