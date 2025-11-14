#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <execinfo.h>
#include <cheriintrin.h>
#include <signal.h>

#include "stackScan.h"
#include "library.h"
#include <sys/queue.h>


void print_obj(void* object){

	Obj_Entry* obj = (Obj_Entry*)object;

	printf("Magic: %lld\nVersion: %lld\npath: %s\n", obj->magic, obj->version, obj->path);

	printf("\nnext ptr: %#p\n\n", TAILQ_NEXT(obj, next));

	Obj_Entry* obj_next = TAILQ_NEXT(obj, next);

	seenCapabilities *newHead = malloc(sizeof(seenCapabilities));
	Obj_Entry* obj_last;
	do{	
		printf("Magic: %lld\nVersion: %lld\npath: %s\n", obj_next->magic, obj_next->version, obj_next->path);

		void** test = (void**) obj_next;

  			test += 6;
			void* map = *test;

			seenCapabilities *head = malloc(sizeof(seenCapabilities));
		
			head->next = NULL;
			head->capability = head;

			scan_recursive(map, head, 0);
			printf("Found: \n");
			printList(head);
			obj_last = obj_next;
			newHead = head;
		
		obj_next = TAILQ_NEXT(obj_next, next);
		
	} while(obj_next != NULL);

	printf("%#p\n", newHead->capability);

	printf("index 0: %#p\n", getIndex(newHead, 0));

	char* keyAddr = (char*)cheri_address_set(newHead->capability, cheri_base_get(newHead->capability));

	printf("Found: %#p\n", keyAddr);

	printf("testing strings...\n");
	char* testString = "-----BEGIN RSA PRIVATE KEY-----";

	for(int i = 0; i < 0x200000; i++){
	
		char* testKey = keyAddr+i;
		
		if(strlen(testKey) < 100)
			continue;
		if(strncmp(testKey, testString, strlen(testString)) == 0)
			printf("%#p:\n %s\n", testKey, testKey);
	} 


	
	/*


	printf("\nGoing backwards\n");
	TAILQ_HEAD(tailhead, OBJ_ENTRY);

	obj_next = TAILQ_PREV(obj, tailhead, next);
	
	do{	
		printf("Magic: %lld\nVersion: %lld\npath: %s\n", obj_next->magic, obj_next->version, obj_next->path);
		obj_last = obj_next;	

		obj_next = TAILQ_PREV(obj_next, tailhead, next);

	} while(obj_next != NULL);


	void** test = (void**) obj_last;
  	test += 6;
	void* map = *test;

	seenCapabilities *head = malloc(sizeof(seenCapabilities));
		
	head->next = NULL;
	head->capability = head;

	scan_recursive(map, head, 0);
	printf("Found: \n");
	printList(head);

	*/

	return;
}


int test(){

	void* lib = dlopen("libc.so.7", RTLD_NOW);

	printf("lib: %#p\n", (void* __capability) lib);

	print_obj(lib);

	
	

	return 0;

}
