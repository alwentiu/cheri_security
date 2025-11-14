#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <execinfo.h>

#include <signal.h>

#include <cheriintrin.h>

#include "library.h"
#include <sys/queue.h>


void*** heapList;
int size = 220000;

int test(){

	printf("enabled: %d\n", malloc_revoke_enabled());

	heapList = malloc(sizeof(void*) * size);

	for(int i = 0; i < size; i++){
		void*** new = malloc(0x10);
		heapList[i] = new; 
		free(new);
		//printf("%#p\n", new);
	}

	printf("saved %#p\n", heapList[0]);

}


void testStore(void* alloc){

	printf("read: %#p\n", heapList[0]);

	for(int i = 0; i < size; i++){
		//printf("%#p\n", heapList[i]);

		if(alloc == heapList[i]){
			printf("found %#p: %#p\n", heapList[i], *heapList[i]);
		}
	}

}

