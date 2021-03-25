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
int sf_valid_pointer(void *pointer);
void sf_remove_freelist(sf_block * block);
void sf_coalesce(sf_block *block);

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
	//check if pointer valid(if not call abort())
	if(sf_valid_pointer(pp) != 0)
		abort();
	//free the block and try to coalese
	sf_block *block = (sf_block *)(pp - 8);
	size_t size = (block -> header) & ~0x3;

	//change block to free
	block -> header = ((block -> header) & ~THIS_BLOCK_ALLOCATED);

	//set footer

	//edit next block
	sf_block *next = (sf_block *)((void *)block + size);
	next -> header = (next -> header & ~ PREV_BLOCK_ALLOCATED);

	sf_coalesce(block);


    return;
}

void *sf_realloc(void *pp, size_t rsize) {
	//valid pointer?
	// debug("realloc start");
	if(sf_valid_pointer(pp) != 0){
		sf_errno = EINVAL;
		return NULL;
	}
	//size == 0?
	if(rsize == 0){
		free(pp);
		return NULL;
	}

	size_t padding = (16-((rsize + 8)%16))%16;
	size_t updated_size = 32;
	if(rsize > 24)
		updated_size =rsize + 8 + padding;

	sf_block *realloc_block = pp - 8;
	size_t original_size = realloc_block -> header & ~0x3;

	if(original_size > updated_size){
		//split and free
		return sf_split(realloc_block, updated_size);
	}
	else if (updated_size > original_size){
		sf_block *block = sf_malloc(updated_size);
		if(block == NULL)
			return NULL;
		else{
			free(pp); //free original block
			return block -> body.payload;
		}
	}
	return pp; //size never changed
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
		size_t old_size = ((sf_block *)((void *)page - 16)) -> header & ~0x3;
		//(((sf_block *)((char  *)original_epilogue -> header - 8))-> header & ~0x3)
		sf_block *wilderness_block = (sf_block *) ((void *)original_epilogue - old_size);
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
		index = 7;
	//add it to he begining of the sf_free_list_heads[index]
	sf_block* free_list = sf_free_list_heads[index].body.links.next;
	block -> body.links.next = free_list;
	free_list -> body.links.prev = block;
	block -> body.links.prev = &sf_free_list_heads[index];
	sf_free_list_heads[index].body.links.next = block;

}

int sf_valid_pointer(void *pointer){//-1 if not valid; 0 if valid
	debug("valid pointer");
	if(pointer == NULL){
		debug("null pointer");
		return -1;
	}

	//pointer isnt 16 byte aligned
	if((size_t) pointer %16 != 0){
		debug("16 aligned");
		return -1;
	}

	sf_block *block = (sf_block *)((void *)pointer - 8);
	size_t size = block -> header & ~0x3;

	//not a multiple of 16 (also check at least 32)
	if(size < 32 || size % 16 != 0 || !(block -> header & THIS_BLOCK_ALLOCATED)){
		debug("here1\n");
		return -1;
	}

	//pointer header or footer not within range
	if ((void *)block <= sf_mem_start() || (void *)(block + (block -> header)) >= sf_mem_end()){
		debug("here2\n");
		return -1;
	}

	//make sure prev_alloc bit matches alloc bit of prev block
	if(!(block -> header & PREV_BLOCK_ALLOCATED) && (*((sf_header *)(pointer - 16)) & PREV_BLOCK_ALLOCATED) != 0){
		debug("here3\n");
		return -1;
	}

	// debug("good pointer");
	return 0;
}

void sf_remove_freelist(sf_block * block){
	sf_block *next_block = block -> body.links.next;
	block -> body.links.prev -> body.links.next = next_block;
	next_block -> body.links.prev = block -> body.links.prev;

}

void sf_coalesce(sf_block *block){
	size_t size = (block -> header) & ~0x3;
	sf_block *next_block = (sf_block *)((void *)block + size);
	sf_block *prev_block = NULL;

	int next_free = 0, prev_free = 0;

	if(((block -> header) & PREV_BLOCK_ALLOCATED) == 0){ //make sure prev isnt prologue
		prev_block = (sf_block *)((void *)block - ((void *)((block - 8)-> header & ~0x3)));
		prev_free = 1;
	}

	if(((next_block -> header) & THIS_BLOCK_ALLOCATED) == 0) //make sure its not epilogue
		next_free = 1;

	// debug("%d,%d", prev_free, next_free);
	// debug("getting ready to coalese\n");

	size_t size_prev = 0, size_next = 0;
	//prev and next free
	if(prev_free == 1 && next_free == 1){
		size_prev = (prev_block -> header) & ~0x3;
		size_next = (next_block -> header) & ~0x3;

		sf_remove_freelist(prev_block);
		sf_remove_freelist(next_block);

		size_t header_manip = prev_block -> header & PREV_BLOCK_ALLOCATED;

		prev_block -> header = ((size_prev + size + size_next)| header_manip);
		//set footer of blook
		*(sf_header *)((char *)prev_block + ((~0x3 & prev_block -> header) - 8)) = prev_block -> header;
		sf_add_freelist(prev_block);//add block into approprate location
		// debug("prev and next\n");
		return;
	}
	//prev free
	else if(prev_free == 1 && next_free == 0){
		size_prev = (prev_block -> header) & ~0x3;

		sf_remove_freelist(prev_block);

		size_t header_manip = prev_block -> header & PREV_BLOCK_ALLOCATED;

		prev_block -> header = ((size_prev + size)| header_manip);
		//set footer of blook
		*(sf_header *)((char *)prev_block + ((~0x3 & prev_block -> header) - 8)) = prev_block -> header;
		sf_add_freelist(prev_block);//add block into approprate location
		// debug("prev \n");
		return;
	}
	//next free
	else if(prev_free == 0 && next_free == 1){
		size_next = (next_block -> header) & ~0x3;

		sf_remove_freelist(next_block);
		size_t header_manip = block -> header & PREV_BLOCK_ALLOCATED;
		block -> header = ((size_next + size)| header_manip);

		*(sf_header *)((char *)block + ((~0x3 & block -> header) - 8)) = block -> header;
		sf_add_freelist(block);//add block into approprate location
		// debug("next\n");
		return;
	}

	//if nearby blocks not free add
	// debug("neither\n");
	block -> header = (size | ((block -> header) & PREV_BLOCK_ALLOCATED));
	*(sf_header *)((char *)block + ((~0x3 & block -> header) - 8)) = block -> header;
	sf_add_freelist(block);
	return;
}
