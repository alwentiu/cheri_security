#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <execinfo.h>

#include <signal.h>

#include "library.h"
#include <sys/queue.h>

void* testLib(){
	return dlopen("libc.so.7", RTLD_NOW);
}


void print_obj(void* object){

	Obj_Entry* obj = (Obj_Entry*)object;

	printf("Magic: %lld\nVersion: %lld\npath: %s\n", obj->magic, obj->version, obj->path);

	printf("\nnext ptr: %#p\n\n", TAILQ_NEXT(obj, next));

	Obj_Entry* obj_next = TAILQ_NEXT(obj, next);
	do{	
		printf("Magic: %lld\nVersion: %lld\npath: %s\n", obj_next->magic, obj_next->version, obj_next->path);

		printf("Path: %s: %#p\n", obj_next->path, obj_next->mapbase);
		printf("Map base: %#p\n", obj_next->mapbase);	
		obj_next = TAILQ_NEXT(obj_next, next);

	} while(obj_next != NULL);


	printf("\nGoing backwards\n");
	TAILQ_HEAD(tailhead, OBJ_ENTRY);

	obj_next = TAILQ_PREV(obj, tailhead, next);
	do{	
		printf("Magic: %lld\nVersion: %lld\npath: %s\n", obj_next->magic, obj_next->version, obj_next->path);
		
		printf("Map base: %#p\n", obj_next->mapbase);	
		printf("Path: %s: %#p\n", obj_next->path, obj_next->mapbase);

		obj_next = TAILQ_PREV(obj_next, tailhead, next);

	} while(obj_next != NULL);


	return;
}


int test(){

	void* lib2 = dlopen("library2.so", RTLD_NOW);
	printf("lib2: %p\n", (void* __capability) lib2);

	void (*func_test)();
	*(void**)(&func_test) = dlsym(lib2, "test2");
	func_test();

	printf("func test: %#p\n", func_test);

	void* lib = dlopen("libc.so.7", RTLD_NOW);

	printf("lib: %#p\n", (void* __capability) lib);

	print_obj(lib);

	return 0;

}
