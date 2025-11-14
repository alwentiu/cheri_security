#include <stdio.h>

#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>


#include <signal.h>

#include "library.h"
#include <sys/mman.h>


// internal structs

#define GRP_SIZE 32

typedef struct group {
	struct meta *meta;
	unsigned char active_idx:5;
	char pad[GRP_SIZE - sizeof(struct meta *) - 1];
	unsigned char storage[];
} group;

typedef struct meta {
	struct meta *prev, *next;
	struct group *mem;
	volatile int avail_mask, freed_mask;
	size_t last_idx:5;
	size_t freeable:1;
	size_t sizeclass:6;
	size_t maplen:8*sizeof(size_t)-12;
} meta; 

typedef struct meta_area {
	uint64_t check;
	struct meta_area *next;
	int nslots;
	struct meta slots[];
} meta_area;


// https://git.morello-project.org/morello/musl-libc/-/blob/morello/master/src/malloc/mallocng/malloc.c

int test(int** oldArray){

	void** metaMap = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	void** groupMap = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	printf("metadata page: %p\n", metaMap);
	printf("group: %p\n", groupMap);

	group* newGroup = (group*) groupMap;
	meta* newMeta = (meta*) metaMap;

	newGroup->meta = newMeta;

	newMeta->prev = newMeta;
	newMeta->next = newMeta;

	newMeta->group = newGroup;

	printf("%p:%p\n", newGroup, newGroup->meta);

}
