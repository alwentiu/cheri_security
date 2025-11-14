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

#include <errno.h>


typedef struct seenCapabilities{

    void* capability;
    struct seenCapabilities* next;

} seenCapabilities;



int isInList(void* new, seenCapabilities* head){

    ptraddr_t new_base = cheri_base_get(new);
    ptraddr_t new_end = new_base + cheri_length_get(new);

    seenCapabilities* current = head;

    while(current != NULL){
        ptraddr_t cur_base = cheri_base_get(current->capability);
        ptraddr_t cur_end = cur_base + cheri_length_get(current->capability);
    
        // test if new capability is already found
        // | ------ |
        //    | --- | <- already found, exit
        if(new_base > cur_base && new_base < cur_end && new_end <= cur_end){
            //printf("0: %#p, %#p\n", new, current);
            return 1; // found
        }
        // | ------ |
        // | --- | <- already found, exit
        if(new_end > cur_base && new_end < cur_end && new_base >= cur_base){
            //printf("1: %#p, %#p\n", new, current);
            return 1;
        }
        // same capability
        if(new_end == cur_end && new_base == cur_base){
            return 1;
        }

        //  | ------ |
        // | -------- | <- supercedes, so replace
        if(new_base < cur_base && new_end > cur_end){
            current->capability = new;
            return 0;
        }

        // not seen yet, keep looking
        current = current->next;
    }

    //add new if not found
    seenCapabilities* new_store = malloc(sizeof(seenCapabilities));

    seenCapabilities* old_second = head->next;

    new_store->next = old_second;
    new_store->capability = new;
    head->next = new_store;

    return 0;
}


void printList(seenCapabilities* head){

    seenCapabilities* current = head;

    while(current != NULL){
        printf("\t%#p\n", current->capability);
        current = current->next;
    }

}


void scan_recursive(void* cap, seenCapabilities* seenHead, int print_cap){
    size_t len = cheri_length_get(cap);
    printf("len: %lu\n", len);

    if(len < sizeof(void*)){
        printf("too small, exiting...\n");
        return;
    }

    cap = cheri_address_set(cap, cheri_base_get(cap));

    // try to find sealer in entire address space
    for(int i = 0; i < len; i+=sizeof(void*)){
        access((char*)(cap+i), 0);
        if(errno == 14){
            //printf("%#p Segfault, avoiding address\n", (cap+i));
            continue;
        }

        void* __capability new_cap = *((void** __capability)(cap+i));
        if(print_cap){
            printf("%p: %#p\n", (cap+i), new_cap);
        }

        if(cheri_length_get(new_cap) < sizeof(void*) * 3){
            continue;
        }

        size_t perms = cheri_perms_get(new_cap);

        if(cheri_tag_get(new_cap) && (perms & CHERI_PERM_LOAD_CAP) == CHERI_PERM_LOAD_CAP)
	{
            printf("Read perm: %p: %#p\n", (cap+i), new_cap);

            if(!isInList(new_cap, seenHead)){
                printf(" - new cap, recursing: %#p\n", new_cap);

                int testIn;
                //scanf("%d", &testIn);

                //if(testIn == 0)
                scan_recursive(new_cap, seenHead, print_cap);
            }
        }
    }
}




int test(){

	// get csp
	printf("starting stack scan\n");

	void* csp;
	asm(
	"mov %[reg], csp\n"
	: [reg] "=r" (csp)
	:
	:
	);

	printf("csp: %#p, revoke enabled: %d\n", csp, malloc_revoke_enabled());

	seenCapabilities* head = malloc(sizeof(seenCapabilities));

    	head->next = NULL;
  	head->capability = head;

	scan_recursive(csp, head, 0);

	printList(head);
}

