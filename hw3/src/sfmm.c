/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

void sf_initialize();

void *sf_malloc(size_t size) {
    if (size == 0)
    	return NULL;
    //calculate how much space need to allocate(padding)

}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    return NULL;
}

void sf_initialize(){
	for(int i = 0; i < NUM_FREE_LISTS; i++){
		sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
		sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
	}
	sf_block *block = (sf_block*)sf_mem_grow;


}