#include <stdlib.h>
#include <stdio.h>

#include "bdd.h"
#include "debug.h"

/*
 * Macros that take a pointer to a BDD node and obtain pointers to its left
 * and right child nodes, taking into account the fact that a node N at level l
 * also implicitly represents nodes at levels l' > l whose left and right children
 * are equal (to N).
 *
 * You might find it useful to define macros to do other commonly occurring things;
 * such as converting between BDD node pointers and indices in the BDD node table.
 */
#define LEFT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->left)
#define RIGHT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->right)
int bdd_node_table_counter = 256;

int nodeEqual(BDD_NODE *node, char level, int left, int right);
int hashIndex(int level, int left, int right);
int bfr_recursion_helper(int wstart, int wend, int hstart, int hend, int w, int h, unsigned char *raster);

/**
 * Look up, in the node table, a BDD node having the specified level and children,
 * inserting a new node if a matching node does not already exist.
 * The returned value is the index of the existing node or of the newly inserted node.
 *
 * The function aborts if the arguments passed are out-of-bounds.
 */
int bdd_lookup(int level, int left, int right) {
    //invalid args check
    if (left < 0 || left > BDD_NODES_MAX)
        return -1;
    if (level < 0 || level > BDD_NODES_MAX)
        return -1;
    if (right < 0 || right > BDD_NODES_MAX)
        return -1;
    if (left == right)
        return left;

    int index = hashIndex(level, left, right);

    while (*(bdd_hash_map + index) != NULL){
        if(nodeEqual(*(bdd_hash_map + index), level, left, right) == 1)
            return *(bdd_hash_map + index) - bdd_nodes;
        index++;
        if(index == BDD_HASH_SIZE) //looping around if reached the end
            index = 0;
    }

    //create node
    BDD_NODE *new_node = bdd_nodes + bdd_node_table_counter;
    new_node -> level = level;
    new_node -> left = left;
    new_node -> right = right;

    *(bdd_hash_map + index) = new_node;
    return bdd_node_table_counter++;
}

int hashIndex(int level, int left, int right){
    int index = level & 0x7f;
    index = index << 7;
    index = index | (left & 0x7f);
    index = index <<7;
    index = index | (right & 0x7f);
    return index % BDD_HASH_SIZE;
}

int nodeEqual(BDD_NODE *node, char level, int left, int right){
    if (node -> level == level && node -> left == left && node -> right == right)
        return 1;
    return 0;
}

int bdd_min_level(int w, int h){
    if (w < 1 || h < 1)
        return -1; // invalid

    int max = w;
    if (h > w)
        max = h;
    //base cases
    if (max == 1)
        return 0;
    else if (max == 2)
        return 2;

    int level = 2;
    int size = 2;
    while (size < max){
        size *= 2;
        level += 2;
    }

    return level;
}

BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster) {
    if (w <= 0 || h <= 0)
        return NULL; // invalid
    //calculate square size of (w, h)
    int power = bdd_min_level(w, h);
    power /= 2;
    int square = 1;
    for (int i = 0; i < power; i++){
        square *= 2;
    }
    //call recursive func
    int index = bfr_recursion_helper(0, square, 0, square, w, h, raster);
    return bdd_nodes + index;

}

int bfr_recursion_helper(int wstart, int wend, int hstart, int hend, int w, int h, unsigned char *raster){
    int width = wend - wstart;
    int height = hend - hstart;
    if(width * height == 1){
        if(wstart >= w || hstart >= h)
            return 0;
        return *(raster + (hstart * w) + wstart);
    }
    else{
        //continues to recursively call
        int w_midpoint = (wend - wstart)/2;
        int h_midpoint = (hend - hstart)/2;
        int level = bdd_min_level(width, height);

        int top_left = bfr_recursion_helper(wstart, w_midpoint, hstart, h_midpoint, w, h, raster);
        int top_right = bfr_recursion_helper(w_midpoint, wend, hstart, h_midpoint, w, h, raster);
        int top_half = bdd_lookup(level, top_left, top_right);

        int bottom_left = bfr_recursion_helper(wstart, w_midpoint, h_midpoint, hend, w, h, raster);
        int bottom_right = bfr_recursion_helper(w_midpoint, wend, h_midpoint, hend, w, h, raster);
        int bottom_half = bdd_lookup(level, bottom_left, bottom_right);

        return bdd_lookup(level, top_half, bottom_half);
    }
}

void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster) {
    // TO BE IMPLEMENTED
}

int bdd_serialize(BDD_NODE *node, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

BDD_NODE *bdd_deserialize(FILE *in) {
    // TO BE IMPLEMENTED
    return NULL;
}

unsigned char bdd_apply(BDD_NODE *node, int r, int c) {
    // TO BE IMPLEMENTED
    return 0;
}

BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char)) {
    // TO BE IMPLEMENTED
    return NULL;
}

BDD_NODE *bdd_rotate(BDD_NODE *node, int level) {
    // TO BE IMPLEMENTED
    return NULL;
}

BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor) {
    // TO BE IMPLEMENTED
    return NULL;
}
