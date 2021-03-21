/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

void sf_initialize();
sf_block *sf_search_freelist(size_t size);
int sf_page_grow();
sf_block *sf_split(sf_block *block, size_t size);
void sf_add_freelist(sf_block *block);

void *sf_malloc(size_t size) {
    if (size <= 0)
    	return NULL;
    //calculate how much space need to padding (word align)
    size_t padding = (16-((size+8)%16))%16;
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

    while(block == NULL){
    	if(sf_page_grow() == 0)
    		block = sf_search_freelist(updated_size);
    	else{
    		sf_errno = ENOMEM;
    		return NULL;
    	}
    }

    sf_block *free_space = NULL;
    if(block != NULL){
    	if((block-> header & ~0x3) - updated_size >= 32)
    		free_space = sf_split(block, updated_size);
    	else
    		free_space = block;
    }

    //return
    if(free_space != NULL)
    	return free_space -> body.payload;
    else
    	sf_errno = ENOMEM;

    return NULL;
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
	sf_block *second_block = sf_mem_start() + (PAGE_SZ - 8);

	//initialize prolougue
	block -> header = 32;
	block -> header = block -> header | THIS_BLOCK_ALLOCATED;

	//initialize epilogue
	second_block -> header = 0;
	second_block -> header = second_block -> header | THIS_BLOCK_ALLOCATED;


	//connect it to sf_free_list_heads[7] (wilderness block)
	sf_block *body = (sf_block *) (((char *) block) + 32);
	body -> header = PAGE_SZ - 48;

	body -> header = body -> header | PREV_BLOCK_ALLOCATED;
	sf_free_list_heads[7].body.links.next = body;
	sf_free_list_heads[7].body.links.prev = body;
	body -> body.links.next = &sf_free_list_heads[7];
	body -> body.links.prev = &sf_free_list_heads[7];

	*(sf_header *)((char *)body + ((~0x3 & body -> header) - 8)) = body -> header;

}

sf_block *sf_search_freelist(size_t size){
	for(int i = 0; i < NUM_FREE_LISTS; i++){
		sf_block *first = &sf_free_list_heads[i];
		sf_block *block = first -> body.links.next;
		while(block != first){
			if(size <= (block -> header & -0x3)){
				block -> body.links.prev -> body.links.next = block ->body.links.next;
				block -> body.links.next -> body.links.prev = block -> body.links.prev;
				return block;
			}
			block = block -> body.links.next;
		}
	}
	return NULL;
}

int sf_page_grow(){
	sf_block *page = (sf_block *)sf_mem_grow();
	if(page == NULL)
		return -1;
	sf_block *original_epilogue = (sf_block *)(sf_mem_end() - PAGE_SZ - 8);
	sf_block *new_epilogue = (sf_block *)(sf_mem_end() - 8);
	if((original_epilogue -> header & PREV_BLOCK_ALLOCATED) == 0){
		sf_block *wilderness_block = (sf_block *) (original_epilogue - (((sf_block *)(original_epilogue -> header - 8))-> header & 0x3));
		wilderness_block -> header = wilderness_block -> header + PAGE_SZ;
		*(sf_header *)((char *)wilderness_block + ((~0x3 & wilderness_block -> header) - 8)) = wilderness_block -> header;
	}
	//make new epilogue to original
	new_epilogue -> header = original_epilogue -> header;
	return 0;

}

sf_block *sf_split(sf_block *block, size_t size){
	sf_block *split_block = (sf_block *)((char *)block + size);
	split_block -> header = ((block -> header & ~0x3) - size) | PREV_BLOCK_ALLOCATED;
	*(sf_header *)((char *)split_block + ((~0x3 & split_block -> header) - 8)) = split_block -> header;
	sf_add_freelist(split_block);


	block -> header = (block -> header & PREV_BLOCK_ALLOCATED) | size | THIS_BLOCK_ALLOCATED;

	return block;
}

void sf_add_freelist(sf_block *block){
	size_t size = (block -> header & ~0x3);
	int index = 32;
	for(int i = 0; i < NUM_FREE_LISTS; i++){
		if(size <= index){
			index = i;
			break;
		}
		index = index << 1;
	}

	if(index >= NUM_FREE_LISTS)
		index = 7;

	if(block == sf_mem_end() - 8 - size)
		index =7;
	sf_block* free_list = sf_free_list_heads[index].body.links.next;
	block -> body.links.next = free_list;
	free_list -> body.links.prev = block;
	block -> body.links.prev = &sf_free_list_heads[index];
	sf_free_list_heads[index].body.links.next = block;

}
