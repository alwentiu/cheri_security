/*
 * stackScan.c -- ../sslExample's scan with access() syscall removed from
 */

#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <execinfo.h>

#include <signal.h>

#include <cheriintrin.h>

#include "stackScan.h"
#include <sys/queue.h>

#include <errno.h>
#include <setjmp.h>
#include <unistd.h>

/* --- fault-tolerant capability load ---------------------------------------
 * No probe. The load is attempted and the handler picks up the pieces. */
static sigjmp_buf __scan_env;
static volatile int __scan_faulted;
static unsigned long long __scan_faults;	/* pages skipped via a fault  */
static unsigned long long __scan_caps;	    /* capabilities actually read     */
static unsigned long long __scan_calls;		/* regions actually scanned   */
static unsigned long long __scan_fault_at_zero;	/* first granule faulted      */
static unsigned long long __scan_misaligned;	/* base not 16-byte aligned   */
static unsigned long long __scan_empty;		/* regions that read nothing  */
static unsigned long long __scan_entries;	/* worker invocations         */
static unsigned long long __scan_toplevel;	/* public entry calls         */
static unsigned long long __scan_installs;	/* times sigaction actually ran */
static size_t __scan_pagesize;

static void __scan_fault_handler(int sig){ (void)sig; __scan_faulted = 1; siglongjmp(__scan_env, 1); }

static void __scan_stats(void){
    printf("\n=== scan cost ===\n");
    printf("  caps read          : %llu\n", __scan_caps);
    printf("  faults taken           : %llu\n", __scan_faults);
    printf("  syscalls in loop       : 0\n");
    printf("  regions scanned        : %llu\n", __scan_calls);
    printf("  first load faulted     : %llu\n", __scan_fault_at_zero);
    printf("  misaligned base        : %llu\n", __scan_misaligned);
    printf("  regions read 0         : %llu\n", __scan_empty);
    printf("  entry-point calls      : %llu\n", __scan_toplevel);
    printf("  worker invocations     : %llu\n", __scan_entries);
    printf("  sigaction installs     : %llu\n", __scan_installs);
    fflush(stdout);
}

static void __scan_install_handlers(void){
    static int done = 0; if(done) return; done = 1;
    __scan_installs++;
    struct sigaction sa; sa.sa_handler = __scan_fault_handler;
    sigemptyset(&sa.sa_mask); sa.sa_flags = SA_NODEFER;
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
#ifdef SIGPROT
    sigaction(SIGPROT, &sa, NULL);
#endif
    __scan_pagesize = (size_t)sysconf(_SC_PAGESIZE);
    if(__scan_pagesize == 0) __scan_pagesize = 4096;
    atexit(__scan_stats);
}


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

    int index = 0;

    while(current != NULL){

        printf("\t%d: %#p\n", index, current->capability);
        current = current->next;
 	index++;
   }

}


void * getIndex(seenCapabilities* head, int index){

    seenCapabilities* current = head;

    int curIndex = 0;

    while(current != NULL && curIndex != index){
        current = current->next;
 	curIndex++;
    }
    return current;
}

/*
 * The recursive worker. It assumes the fault handler is already installed and
 * __scan_pagesize is already set, which is the public entry point's job below.
 *
 */
static void scan_recursive_inner(void* cap, seenCapabilities* seenHead, int print_cap){
    __scan_entries++;
    size_t len = cheri_length_get(cap);
    ptraddr_t base;
    size_t pg = __scan_pagesize;

    /*
     * i is written between the sigsetjmp and the siglongjmp, so it has to be
     * volatile or its value after the jump is indeterminate. Nothing else in
     * this frame is modified after the jump target is armed.
     */
    volatile size_t i = 0;

    if(len < sizeof(void*)){
        printf("too small, exiting...\n");
        return;
    }

    if(len < 160){
	return;
    }

    cap = cheri_address_set(cap, cheri_base_get(cap));
    base = cheri_address_get(cap);
    __scan_calls++;
    if((base & (sizeof(void*) - 1)) != 0)
        __scan_misaligned++;
    unsigned long long __entry_caps = __scan_caps;

    // Set the jump target for the sig handler to return to. 
    if(sigsetjmp(__scan_env, 0) != 0){
        // This will be run when the signal handler returns.
        // If the page fault happened while dereferencing base+i, that means
        // the page is not mapped, so move on to the next page.
        size_t next = ((base + i + pg) & ~(size_t)(pg - 1)) - base;
        next = (next + sizeof(void*) - 1) & ~(size_t)(sizeof(void*) - 1);
        if(i == 0)
            __scan_fault_at_zero++;
        i = next;
        __scan_faults++;
    }

    // try to find sealer in entire address space
    for(; i < len; i += sizeof(void*)){
        void* __capability new_cap = *((void** __capability)(cap + i));
        __scan_caps++;

        if(print_cap){
            printf("%p: %#p\n", (cap+i), new_cap);
        }

        if(cheri_length_get(new_cap) < sizeof(void*) * 3){
            continue;
        }

        size_t perms = cheri_perms_get(new_cap);

        if(cheri_tag_get(new_cap) && (perms & CHERI_PERM_LOAD_CAP) == CHERI_PERM_LOAD_CAP)
	    {
            if(!isInList(new_cap, seenHead)){

                /*
                 * The recursive call sets __scan_env for its own frame. Save
                 * and restore ours around it, or a fault after the call
                 * returns would jump into a frame that no longer exists.
                 */
                sigjmp_buf saved;
                memcpy(&saved, &__scan_env, sizeof(saved));
                scan_recursive_inner(new_cap, seenHead, print_cap);
                memcpy(&__scan_env, &saved, sizeof(saved));
            }
        }
    }

    if(__scan_caps == __entry_caps)
        __scan_empty++;
}

void scan_recursive(void* cap, seenCapabilities* seenHead, int print_cap){
    __scan_toplevel++;
    __scan_install_handlers();
    scan_recursive_inner(cap, seenHead, print_cap);
}


int testScan(){

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
