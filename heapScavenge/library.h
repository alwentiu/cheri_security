#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include <sys/queue.h>

typedef struct Struct_Obj_Entry {

	uint64_t magic;
	uint64_t version;	
	
	TAILQ_ENTRY(Struct_Obj_Entry) next;
	char *path;
	char* origin_path;
	int refcount;
	int holdcount;
	int dl_refcount;

	vaddr_t mapbase;
	size_t mapsize;

} Obj_Entry;

int test(int** oldArray);
