#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>


#include <signal.h>

#include "library.h"



int test(int** oldArray){

	//printf("enabled: %d\n", malloc_revoke_enabled());
	//malloc_revoke();
	//malloc_revoke_quarantine_force_flush();

	for(int i = 0; i < 1000000; i++){
		int** newChunk = malloc(0x10);
		//printf("%#p\n", newChunk);
		if(newChunk == oldArray){
			printf("%#p\n", newChunk);
			printf("%#p\n", *newChunk);
			printf("%d\n", **newChunk);
			**newChunk = 0x41;
			return 0;
		}

		free(newChunk);
	}

}
