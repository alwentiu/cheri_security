#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include <sys/queue.h>

typedef struct seenCapabilities{

    void* capability;
    struct seenCapabilities* next;

} seenCapabilities;


void scan_recursive(void* cap, seenCapabilities* seenHead, int print_cap);

void printList(seenCapabilities* head);

void * getIndex(seenCapabilities* head, int index);

