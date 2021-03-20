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
sf_block *sf_search_freelist(size_t size);

void *sf_malloc(size_t size) {
    if (size <= 0)
    	return NULL;
    //calculate how much space need to padding (word align)
    size_t padding = 16-((size+8)%16);
    //size to allocate
    size_t updated_size = 32;
    if(size > 24)
    	updated_size = size + 8 + padding;

    //check if lists are initialized
    void *starting_mem = sf_mem_start();
    void *ending_mem = sf_mem_end();
    if(starting_mem == ending_mem)
    	sf_initialize();

    //get pointer to space
    sf_block *block = NULL;
    block = sf_search_freelist(updated_size);




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

	sf_block *block = (sf_block*)(sf_mem_grow() + 8);
	printf("%p\n", block);
	sf_block *second_block = sf_mem_start() + (PAGE_SZ - 16);

	//connect it to sf_free_list_heads[7] (wilderness block)
	sf_free_list_heads[7].body.links.next = block;
	sf_free_list_heads[7].body.links.prev = block;
	block -> body.links.prev = &sf_free_list_heads[7];
	block -> body.links.next = &sf_free_list_heads[7];


	//connect footer of second block to equal header of block
	block -> header = PAGE_SZ - 8;
	second_block -> header = block -> header;
}

sf_block *sf_search_freelist(size_t size){
	for(int i = 0; i < NUM_FREE_LISTS; i++){


	}
}